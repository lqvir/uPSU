// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

// STD
#include <algorithm>
#include <future>
#include <iostream>
#include <sstream>
#include <stdexcept>


// APSU
#include "apsu/log.h"
#include "apsu/network/channel.h"
#include "apsu/plaintext_powers.h"
#include "apsu/receiver.h"
#include "apsu/thread_pool_mgr.h"
#include "apsu/util/db_encoding.h"
#include "apsu/util/label_encryptor.h"
#include "apsu/util/utils.h"

// SEAL
#include "seal/ciphertext.h"
#include "seal/context.h"
#include "seal/encryptionparams.h"
#include "seal/keygenerator.h"
#include "seal/plaintext.h"
#include "seal/util/common.h"
#include "seal/util/defines.h"

#include "kunlun/mpc/oprf/mp_oprf.hpp"

using namespace std;
using namespace seal;
using namespace seal::util;
using namespace kuku;

namespace apsu {
    using namespace util;
    using namespace network;
    using namespace oprf;

    namespace {
        template <typename T>
        bool has_n_zeros(T *ptr, size_t count)
        {
            return all_of(ptr, ptr + count, [](auto a) { return a == T(0); });
        }
        inline oc::block vec_to_oc_block(const std::vector<uint64_t> &in,size_t felts_per_item,uint64_t plain_modulus){
            uint32_t plain_modulus_len = 1;
            while(((1<<plain_modulus_len)-1)<plain_modulus){
                plain_modulus_len++;
            }
            uint64_t plain_modulus_mask = (1<<plain_modulus_len)-1;
            uint64_t plain_modulus_mask_lower = (1<<(plain_modulus_len>>1))-1;
            uint64_t plain_modulus_mask_higher = plain_modulus_mask-plain_modulus_mask_lower;
            // cout<<"masks"<<endl;
            // cout<<hex<<plain_modulus<<endl;
            // cout<<hex<<plain_modulus_mask_lower<<endl;
            // cout<<hex<<plain_modulus_mask_higher<<endl;
            uint64_t lower=0,higher=0;
            if(felts_per_item&1){
                lower = (in[felts_per_item-1] & plain_modulus_mask_lower);
                higher = ((in[felts_per_item-1] & plain_modulus_mask_higher) >>((plain_modulus_len>>1)-1));
            }
            for(int pla = 0;pla < felts_per_item-1;pla+=2){
                lower = ((in[pla] & plain_modulus_mask) | (lower<<plain_modulus_len));
                higher = ((in[pla+1] & plain_modulus_mask) | (higher<<plain_modulus_len));
            }
            return oc::toBlock(higher,lower);
        }

        inline block vec_to_std_block(const std::vector<uint64_t> &in,size_t felts_per_item,uint64_t plain_modulus){
            uint32_t plain_modulus_len = 1;
            while(((1<<plain_modulus_len)-1)<plain_modulus){
                plain_modulus_len++;
            }
            uint64_t plain_modulus_mask = (1<<plain_modulus_len)-1;
            uint64_t plain_modulus_mask_lower = (1<<(plain_modulus_len>>1))-1;
            uint64_t plain_modulus_mask_higher = plain_modulus_mask-plain_modulus_mask_lower;
                //  cout<<"masks"<<endl;
                // cout<<hex<<plain_modulus<<endl;
                // cout<<hex<<plain_modulus_mask_lower<<endl;
                // cout<<hex<<plain_modulus_mask_higher<<endl;
            uint64_t lower=0,higher=0;
            if(felts_per_item&1){
                lower = (in[felts_per_item-1] & plain_modulus_mask_lower);
                higher = ((in[felts_per_item-1] & plain_modulus_mask_higher) >>((plain_modulus_len>>1)-1));
            }
            //cout<< lower<<' '<< higher<<endl;
            for(int pla = 0;pla < felts_per_item-1;pla+=2){
                lower = ((in[pla] & plain_modulus_mask) | (lower<<plain_modulus_len));
                higher = ((in[pla+1] & plain_modulus_mask) | (higher<<plain_modulus_len));
            }
            return Block::MakeBlock(higher,lower);
        }

        inline block block_oc_to_std(oc::block in){
                return Block::MakeBlock(in.as<uint64_t>()[1],in.as<uint64_t>()[0]);
            }
        std::vector<oc::block> decrypt_randoms_matrix;
        std::vector<block> mpoprf_in;
    } // namespace

    namespace receiver {
        size_t IndexTranslationTable::find_item_idx(size_t table_idx) const noexcept
        {
            auto item_idx = table_idx_to_item_idx_.find(table_idx);
            if (item_idx == table_idx_to_item_idx_.cend()) {
                return item_count();
            }

            return item_idx->second;
        }

        Receiver::Receiver(PSIParams params) : params_(move(params))
        {
            initialize();
            
        }

        void Receiver::reset_keys()
        {
            // Generate new keys
            KeyGenerator generator(*get_seal_context());

            // Set the symmetric key, encryptor, and decryptor
            crypto_context_.set_secret(generator.secret_key());

            // Create Serializable<RelinKeys> and move to relin_keys_ for storage
            relin_keys_.clear();
            if (get_seal_context()->using_keyswitching()) {
                Serializable<RelinKeys> relin_keys(generator.create_relin_keys());
                relin_keys_.set(move(relin_keys));
            }
        }

        uint32_t Receiver::reset_powers_dag(const set<uint32_t> &source_powers)
        {
            // First compute the target powers
            set<uint32_t> target_powers = create_powers_set(
                params_.query_params().ps_low_degree, params_.table_params().max_items_per_bin);

            // Configure the PowersDag
            pd_.configure(source_powers, target_powers);

            // Check that the PowersDag is valid
            if (!pd_.is_configured()) {
                APSU_LOG_ERROR(
                    "Failed to configure PowersDag ("
                    << "source_powers: " << to_string(source_powers) << ", "
                    << "target_powers: " << to_string(target_powers) << ")");
                throw logic_error("failed to configure PowersDag");
            }
            APSU_LOG_DEBUG("Configured PowersDag with depth " << pd_.depth());

            return pd_.depth();
        }

        void Receiver::initialize()
        {
            APSU_LOG_DEBUG("PSI parameters set to: " << params_.to_string());
            APSU_LOG_DEBUG(
                "Derived parameters: "
                << "item_bit_count_per_felt: " << params_.item_bit_count_per_felt()
                << "; item_bit_count: " << params_.item_bit_count()
                << "; bins_per_bundle: " << params_.bins_per_bundle()
                << "; bundle_idx_count: " << params_.bundle_idx_count());

            STOPWATCH(recv_stopwatch, "Receiver::initialize");

            // Initialize the CryptoContext with a new SEALContext
            crypto_context_ = CryptoContext(params_);

            // Set up the PowersDag
            reset_powers_dag(params_.query_params().query_powers);

            // Create new keys
            reset_keys();

            // init send Messages
            sendMessages.clear();

        
            
       

        }

        unique_ptr<SenderOperation> Receiver::CreateParamsRequest()
        {
            auto sop = make_unique<SenderOperationParms>();
            APSU_LOG_INFO("Created parameter request");

            return sop;
        }

        PSIParams Receiver::RequestParams(NetworkChannel &chl)
        {
            // Create parameter request and send to Sender
            chl.send(CreateParamsRequest());

            // Wait for a valid message of the right type

            
            ParamsResponse response;
            bool logged_waiting = false;
            while (!(response = to_params_response(chl.receive_response()))) {
                if (!logged_waiting) {
                    // We want to log 'Waiting' only once, even if we have to wait for several
                    // sleeps.
                    logged_waiting = true;
                    APSU_LOG_INFO("Waiting for response to parameter request");
                }

                this_thread::sleep_for(50ms);
            }

            return *response->params;
        }

// oprf has been removed

        OPRFReceiver Receiver::CreateOPRFReceiver(const vector<Item> &items)
        {
            STOPWATCH(recv_stopwatch, "Receiver::CreateOPRFReceiver");

            OPRFReceiver oprf_receiver(items);
            APSU_LOG_INFO("Created OPRFReceiver for " << oprf_receiver.item_count() << " items");

            return oprf_receiver;
        }

        pair<vector<HashedItem>, vector<LabelKey>> Receiver::ExtractHashes(
            const OPRFResponse &oprf_response, const OPRFReceiver &oprf_receiver)
        {
            STOPWATCH(recv_stopwatch, "Receiver::ExtractHashes");

            if (!oprf_response) {
                APSU_LOG_ERROR("Failed to extract OPRF hashes for items: oprf_response is null");
                return {};
            }

            auto response_size = oprf_response->data.size();
            size_t oprf_response_item_count = response_size / oprf_response_size;
            if ((response_size % oprf_response_size) ||
                (oprf_response_item_count != oprf_receiver.item_count())) {
                APSU_LOG_ERROR(
                    "Failed to extract OPRF hashes for items: unexpected OPRF response size ("
                    << response_size << " B)");
                return {};
            }

            vector<HashedItem> items(oprf_receiver.item_count());
            vector<LabelKey> label_keys(oprf_receiver.item_count());
            oprf_receiver.process_responses(oprf_response->data, items, label_keys);
            APSU_LOG_INFO("Extracted OPRF hashes for " << oprf_response_item_count << " items");

            return make_pair(move(items), move(label_keys));
        }
// oprf has been removed
        unique_ptr<SenderOperation> Receiver::CreateOPRFRequest(const OPRFReceiver &oprf_receiver)
        {
            auto sop = make_unique<SenderOperationOPRF>();
            sop->data = oprf_receiver.query_data();
            APSU_LOG_INFO("Created OPRF request for " << oprf_receiver.item_count() << " items");

            return sop;
        }
// oprf has been removed
        pair<vector<HashedItem>, vector<LabelKey>> Receiver::RequestOPRF(
            const vector<Item> &items, NetworkChannel &chl)
        {
            auto oprf_receiver = CreateOPRFReceiver(items);

            // Create OPRF request and send to Sender
            chl.send(CreateOPRFRequest(oprf_receiver));

            // Wait for a valid message of the right type
            OPRFResponse response;
            bool logged_waiting = false;
            while (!(response = to_oprf_response(chl.receive_response()))) {
                if (!logged_waiting) {
                    // We want to log 'Waiting' only once, even if we have to wait for several
                    // sleeps.
                    logged_waiting = true;
                    APSU_LOG_INFO("Waiting for response to OPRF request");
                }

                this_thread::sleep_for(50ms);
            }

            // Extract the OPRF hashed items
            return ExtractHashes(response, oprf_receiver);
        }

        pair<Request, IndexTranslationTable> Receiver::create_query(const vector<HashedItem> &items,const std::vector<string> &origin_item)
        {
            APSU_LOG_INFO("Creating encrypted query for " << items.size() << " items");
            STOPWATCH(recv_stopwatch, "Receiver::create_query");
            all_timer.setTimePoint("create_query");
            IndexTranslationTable itt;
            itt.item_count_ = items.size();

            // Create the cuckoo table
            KukuTable cuckoo(
                params_.table_params().table_size,      // Size of the hash table
                0,                                      // Not using a stash
                params_.table_params().hash_func_count, // Number of hash functions
                { 0, 0 },                               // Hardcoded { 0, 0 } as the seed
                cuckoo_table_insert_attempts,           // The number of insertion attempts
                { 0, 0 });                              // The empty element can be set to anything

            // Hash the data into a cuckoo hash table
            // cuckoo_hashing
            {
                STOPWATCH(recv_stopwatch, "Receiver::create_query::cuckoo_hashing");
                APSU_LOG_DEBUG(
                    "Inserting " << items.size() << " items into cuckoo table of size "
                                 << cuckoo.table_size() << " with " << cuckoo.loc_func_count()
                                 << " hash functions");
                for (size_t item_idx = 0; item_idx < items.size(); item_idx++) {
                    const auto &item = items[item_idx];
                    if (!cuckoo.insert(item.get_as<kuku::item_type>().front())) {
                        // Insertion can fail for two reasons:
                        //
                        //     (1) The item was already in the table, in which case the "leftover
                        //     item" is empty; (2) Cuckoo hashing failed due to too small table or
                        //     too few hash functions.
                        //
                        // In case (1) simply move on to the next item and log this issue. Case (2)
                        // is a critical issue so we throw and exception.
                        if (cuckoo.is_empty_item(cuckoo.leftover_item())) {
                            APSU_LOG_INFO(
                                "Skipping repeated insertion of items["
                                << item_idx << "]: " << item.to_string());
                        } else {
                            APSU_LOG_ERROR(
                                "Failed to insert items["
                                << item_idx << "]: " << item.to_string()
                                << "; cuckoo table fill-rate: " << cuckoo.fill_rate());
                            throw runtime_error("failed to insert item into cuckoo table");
                        }
                    }
                }
                APSU_LOG_DEBUG(
                    "Finished inserting items with "
                    << cuckoo.loc_func_count()
                    << " hash functions; cuckoo table fill-rate: " << cuckoo.fill_rate());
            }
          
            sendMessages.assign(cuckoo.table_size(),{oc::ZeroBlock,oc::ZeroBlock});

            shuffleMessages.assign(cuckoo.table_size(),{oc::ZeroBlock,oc::ZeroBlock});
            // Once the table is filled, fill the table_idx_to_item_idx map
            for (size_t item_idx = 0; item_idx < items.size(); item_idx++) {
                auto item_loc = cuckoo.query(items[item_idx].get_as<kuku::item_type>().front());
                auto temp_loc = item_loc.location();
                itt.table_idx_to_item_idx_[temp_loc] = item_idx;
               // sendMessages[temp_loc]={oc::ZeroBlock,oc::toBlock((uint8_t*)origin_item[item_idx].data())};
                sendMessages[temp_loc]={oc::toBlock((uint8_t*)origin_item[item_idx].data()),oc::ZeroBlock};
                // APSU_LOG_INFO(sendMessages[temp_loc][0].as<char>().data());
               // cout<<(int)sendMessages[temp_loc][0].as<char>()[0]<<endl;
            }

            // Set up unencrypted query data
            vector<PlaintextPowers> plain_powers;

            // prepare_data
            {
                STOPWATCH(recv_stopwatch, "Receiver::create_query::prepare_data");
                for (uint32_t bundle_idx = 0; bundle_idx < params_.bundle_idx_count();
                     bundle_idx++) {
                    APSU_LOG_DEBUG("Preparing data for bundle index " << bundle_idx);

                    // First, find the items for this bundle index
                    gsl::span<const item_type> bundle_items(
                        cuckoo.table().data() + bundle_idx * params_.items_per_bundle(),
                        params_.items_per_bundle());

                    vector<uint64_t> alg_items;
                    for (auto &item : bundle_items) {
                        // Now set up a BitstringView to this item
                        gsl::span<const unsigned char> item_bytes(
                            reinterpret_cast<const unsigned char *>(item.data()), sizeof(item));
                        BitstringView<const unsigned char> item_bits(
                            item_bytes, params_.item_bit_count());

                        // Create an algebraic item by breaking up the item into modulo
                        // plain_modulus parts
                        vector<uint64_t> alg_item =
                            bits_to_field_elts(item_bits, params_.seal_params().plain_modulus());
                        std::copy(alg_item.cbegin(), alg_item.cend(), back_inserter(alg_items));
                    }

                    // Now that we have the algebraized items for this bundle index, we create a
                    // PlaintextPowers object that computes all necessary powers of the algebraized
                    // items.
                    plain_powers.emplace_back(move(alg_items), params_, pd_);
                }
                

            }

            // The very last thing to do is encrypt the plain_powers and consolidate the matching
            // powers for different bundle indices
            unordered_map<uint32_t, vector<SEALObject<Ciphertext>>> encrypted_powers;

            // encrypt_data
            {
                STOPWATCH(recv_stopwatch, "Receiver::create_query::encrypt_data");
                for (uint32_t bundle_idx = 0; bundle_idx < params_.bundle_idx_count();
                     bundle_idx++) {
                    APSU_LOG_DEBUG("Encoding and encrypting data for bundle index " << bundle_idx);

                    // Encrypt the data for this power
                    auto encrypted_power(plain_powers[bundle_idx].encrypt(crypto_context_));

                    // Move the encrypted data to encrypted_powers
                    for (auto &e : encrypted_power) {
                        encrypted_powers[e.first].emplace_back(move(e.second));
                    }
                }
            }

            // Set up the return value
            auto sop_query = make_unique<SenderOperationQuery>();
            sop_query->compr_mode = seal::Serialization::compr_mode_default;
            sop_query->relin_keys = relin_keys_;
            sop_query->data = move(encrypted_powers);
            auto sop = to_request(move(sop_query));

            APSU_LOG_INFO("Finished creating encrypted query");
            all_timer.setTimePoint("create_query finish");
            return { move(sop), itt };
        }

        vector<MatchRecord> Receiver::request_query(
            const vector<HashedItem> &items,
         
            NetworkChannel &chl,
            const vector<string> &origin_item)
        {
            ThreadPoolMgr tpm;

            // Create query and send to Sender
            auto query = create_query(items,origin_item);
            chl.send(move(query.first));
            auto itt = move(query.second);
            all_timer.setTimePoint("with response start");

            // Wait for query response
            QueryResponse response;
            bool logged_waiting = false;
            while (!(response = to_query_response(chl.receive_response()))) {
                if (!logged_waiting) {
                    // We want to log 'Waiting' only once, even if we have to wait for several
                    // sleeps.
                    logged_waiting = true;
                    APSU_LOG_INFO("Waiting for response to query request");
                }

                this_thread::sleep_for(50ms);
            }
            all_timer.setTimePoint("with response finish");

                uint32_t bundle_idx_count = safe_cast<uint32_t>(params_.bundle_idx_count()); 
                uint32_t items_per_bundle = safe_cast<uint32_t>(params_.items_per_bundle());
                size_t felts_per_item = safe_cast<size_t>(params_.item_params().felts_per_item);
                uint32_t item_cnt = bundle_idx_count* items_per_bundle; 

        //       int block_num = ((felts_per_item+3)/4);
         

            // {
                
               
               
            //     int numThreads=1;
            //     int pack_cnt =response->package_count;
            //     oc::IOService ios;
            //     oc::Session recv_session=oc::Session(ios,"localhost:59999",oc::SessionMode::Client);
            //     std::vector<oc::Channel> recv_chls(numThreads);

            //     shuffle_size = pack_cnt * items_per_bundle;
            //     APSU_LOG_INFO(pack_cnt);
            //     for(int i=0;i<numThreads;i++)
            //         recv_chls[i] = recv_session.addChannel();
            //     OSNSender osn;
            //     osn.init(shuffle_siz
        
            
           
        //    vector<vector<oc::block> > psi_result_after_shuffle(block_num);
        //     psi_result_before_shuffle.resize(block_num);
        //     for(int i =0;i<block_num;i++){
        //         psi_result_before_shuffle[i].resize(shuffle_size);
        //        psi_result_after_shuffle[i].resize(shuffle_size);
        //     }
          
            // Set up the result
            vector<MatchRecord> mrs(query.second.item_count());

            // Get the number of ResultPackages we expect to receive
            atomic<uint32_t> package_count{ response->package_count };
            

            // prepare decrypt randoms matrix size for copy

            uint32_t alpha_max_cache_count = response->alpha_max_cache_count;
            size_t shuffle_size=alpha_max_cache_count * item_cnt;
            decrypt_randoms_matrix.assign(shuffle_size,oc::ZeroBlock);
            std::vector<oc::block> send_share;
            
            // Launch threads to receive ResultPackages and decrypt results
            size_t task_count = min<size_t>(ThreadPoolMgr::GetThreadCount(), package_count);
            vector<future<void>> futures(task_count);
            APSU_LOG_INFO(
                "Launching " << task_count << " result worker tasks to handle " << package_count
                             << " result parts");
            for (size_t t = 0; t < task_count; t++) {
                futures[t] = tpm.thread_pool().enqueue(
                    [&]() { process_result_worker(package_count, mrs, itt, chl); });
            }

            for (auto &f : futures) {
                f.get();
            }
            vector<int> col_permutation;
            {
                int numThreads=1;
                oc::IOService ios;
                oc::Session recv_session=oc::Session(ios,"localhost:59999",oc::SessionMode::Client);
                std::vector<oc::Channel> recv_chls(numThreads);
                for(int i=0;i<numThreads;i++)
                    recv_chls[i] = recv_session.addChannel();
                OSNSender osn;
                osn.init(alpha_max_cache_count,item_cnt,1,"");
                permutation = osn.dest;
                col_permutation = osn.cols_premutation;
                all_timer.setTimePoint("before osn");
                send_share = osn.run_osn(recv_chls);
                all_timer.setTimePoint("after osn");
                
                send_size=0,receiver_size =0;
                for(auto x: recv_chls){
                    send_size+=x.getTotalDataSent();
                    receiver_size+=x.getTotalDataRecv();
                }
                recv_session.stop();

            }
            std::vector<oc::block> shuffle_decrypt_randoms_matrix(shuffle_size);
    // template shuffle schream
            for(int i = 0;i<shuffle_size;i++){
                shuffle_decrypt_randoms_matrix[i] = decrypt_randoms_matrix[permutation[i]];
            }
            for(int i = 0;i<shuffle_size;i++){
                mpoprf_in.emplace_back(block_oc_to_std((shuffle_decrypt_randoms_matrix[i]^send_share[i])));
            }
            size_t padding = 128-mpoprf_in.size()%128;
            for(int i = 0;i<padding;i++)
                mpoprf_in.emplace_back(Block::all_one_block);
            //APSU_LOG_INFO("padding size"<<mpoprf_in.size());
           // Block::PrintBlocks(mpoprf_in);
            {
            
            Global_Setup(); 
            Context_Initialize(); 
            ECGroup_Initialize(NID_X9_62_prime256v1); 

            NetIO client("client", "127.0.0.1", 59999);
            APSU_LOG_INFO("test");
            //APSU_LOG_INFO(decrypt_randoms_matrix.size()<<item_cnt<<alpha_max_cache_count);
            all_timer.setTimePoint("decrypt finish");
            size_t set_size = mpoprf_in.size();
            size_t log_set_size=int(log2(set_size)+1);

            std::cout<<set_size<<std::endl;
            std::cout<<log_set_size<<std::endl;
            std::string pp_filename = "MPOPRF.pp"; 
            MPOPRF::PP pp; 
           // cout<<set_size<<endl;
            pp = MPOPRF::Setup(log_set_size);
            // if(!FileExist(pp_filename)){
            //     pp = MPOPRF::Setup(log_set_size); // 40 is the statistical parameter
            //     MPOPRF::SavePP(pp, pp_filename); 
            // }
            // else{
            //     MPOPRF::FetchPP(pp, pp_filename); 
            // }
            cout<<pp.log_matrix_height<<endl;
           // APSU_LOG_INFO("test");
            auto mpoprf_key = MPOPRF::Send(client,pp);
           // APSU_LOG_INFO("test1");
            std::vector<std::string> mpoprf_out = MPOPRF::EvaluateOPRFValues(pp,mpoprf_key,mpoprf_in);
            //APSU_LOG_INFO("test2");
 
            //APSU_LOG_INFO(mpoprf_out.size());
            for(int i = 0;i<shuffle_size;i++){
              //  APSU_LOG_INFO(mpoprf_out[i].size());
                client.SendString(mpoprf_out[i]);
            }
            APSU_LOG_INFO("test3");
            ECGroup_Finalize(); 
            Context_Finalize();

            }
           // Block::PrintBlocks(decrypt_randoms_matrix);

            APSU_LOG_INFO("permute"<<permutation.size())   
            for(int i=0;i<item_cnt;i++)
                shuffleMessages[i]=sendMessages[col_permutation[i]];
            all_timer.setTimePoint("decrypt and unpermute finish");
            cout<<all_timer<<endl;
            
            return mrs;
        }

        void Receiver::process_result_part(
        
            const IndexTranslationTable &itt,
            const ResultPart &result_part,
            network::NetworkChannel &chl) const
        {
            STOPWATCH(recv_stopwatch, "Receiver::process_result_part");

            if (!result_part) {
                APSU_LOG_ERROR("Failed to process result: result_part is null");
                return ;
            }

            // The number of items that were submitted in the query
            size_t item_count = itt.item_count();
            
            // Decrypt and decode the result; the result vector will have full batch size
            PlainResultPackage plain_rp = result_part->extract(crypto_context_);
            uint32_t items_per_bundle = safe_cast<uint32_t>(params_.items_per_bundle());
            size_t felts_per_item = safe_cast<size_t>(params_.item_params().felts_per_item);
            vector<block> decrypt_res(items_per_bundle);
            uint64_t plain_modulus=crypto_context_.seal_context()->last_context_data()->parms().plain_modulus().value();
            for(uint32_t item_idx=0;item_idx<items_per_bundle;item_idx++){
                vector<uint64_t> all_felts_one_item(felts_per_item,0);
                for(size_t felts_idx = 0;felts_idx<felts_per_item;felts_idx++){
                    all_felts_one_item[felts_idx] = plain_rp.psi_result[item_idx*felts_per_item+felts_idx];
                }
                decrypt_res[item_idx]= vec_to_oc_block(all_felts_one_item,felts_per_item,plain_modulus);
            }
            uint32_t cache_idx = result_part->cache_idx;
            std::copy(
                decrypt_res.begin(),
                decrypt_res.end(),
                decrypt_randoms_matrix.begin()+(cache_idx*item_count+cache_idx*items_per_bundle)
            );
            
            // {
            //     auto sop_re = make_unique<plainResponse>();
            //     sop_re->bundle_idx = plain_rp.bundle_idx;
               
            //     sop_re->psi_result.resize(plain_rp.psi_result.size());
            //     copy(
            //         plain_rp.psi_result.begin(),
            //         plain_rp.psi_result.end(),
            //         sop_re->psi_result.begin());
            //     //for (auto &i : plain_rp.psi_result) {
            //     //    cout << i << endl;
            //     //}
            //     auto sop = to_request(move(sop_re));
            //     chl.send(move(sop));
            // }
            // size_t felts_per_item = safe_cast<size_t>(params_.item_params().felts_per_item);
            // size_t items_per_bundle = safe_cast<size_t>(params_.items_per_bundle());
            // size_t bundle_start =
            //     mul_safe(safe_cast<size_t>(plain_rp.bundle_idx), items_per_bundle);

            // // Check if we are supposed to have label data present but don't have for some reason
           

            // // Read the nonce byte count and compute the effective label byte count; set the nonce
            // // byte count to zero if no label is expected anyway.
   
            // // If there is a label, then we better have the appropriate label encryption keys
            // // available
  
            // // Set up the result vector
            // vector<MatchRecord> mrs(item_count);

            // cout << item_count << endl
            //      << felts_per_item << endl
            //      << plain_rp.psi_result.size() << endl;
            // cout << "start" << endl;
            // cout << bundle_start << endl;

       

            // // Iterate over the decoded data to find consecutive zeros indicating a match
            // StrideIter<const uint64_t *> plain_rp_iter(plain_rp.psi_result.data(), felts_per_item);
            // seal_for_each_n(iter(plain_rp_iter, size_t(0)), items_per_bundle, [&](auto &&I) {
            //     // Find felts_per_item consecutive zeros
            //     bool match = has_n_zeros(get<0>(I).ptr(), felts_per_item);
            //     if (!match) {
            //         return;
            //     }

            //     // Compute the cuckoo table index for this item. Then find the corresponding index
            //     // in the input items vector so we know where to place the result.
            //     size_t table_idx = add_safe(get<1>(I), bundle_start);
            //     auto item_idx = itt.find_item_idx(table_idx);
              
            //     cout <<plain_rp.bundle_idx<<' '<<result_part->pack_idx<< "match"<<table_idx << endl;
            //     // If this table_idx doesn't match any item_idx, ignore the result no matter what it
            //     // is
            //     if (item_idx == itt.item_count()) {
            //         return;
            //     }

            //     // // If a positive MatchRecord is already present, then something is seriously wrong
            //     // if (mrs[item_idx]) {
            //     //     APSU_LOG_ERROR("The table index -> item index translation table indicated a "
            //     //                    "location that was already filled by another match from this "
            //     //                    "result package; the translation table (query) has probably "
            //     //                    "been corrupted");

            //     //     throw runtime_error(
            //     //         "found a duplicate positive match; something is seriously wrong");
            //     // }

            //     // APSU_LOG_DEBUG(
            //     //     "Match found for items[" << item_idx << "] at cuckoo table index "
            //     //                              << table_idx);

            //     // Create a new MatchRecord
            //     MatchRecord mr;
            //     mr.found = true;

            //     // Next, extract the label results, if any
            //     //if (label_byte_count) {
            //     //    APSU_LOG_DEBUG(
            //     //        "Found " << plain_rp.label_result.size() << " label parts for items["
            //     //                 << item_idx << "]; expecting " << label_byte_count
            //     //                 << "-byte label");

            //     //    // Collect the entire label into this vector
            //     //    AlgLabel alg_label;

            //     //    size_t label_offset = mul_safe(get<1>(I), felts_per_item);
            //     //    for (auto &label_parts : plain_rp.label_result) {
            //     //        gsl::span<felt_t> label_part(
            //     //            label_parts.data() + label_offset, felts_per_item);
            //     //        copy(label_part.begin(), label_part.end(), back_inserter(alg_label));
            //     //    }

            //     //    // Create the label
            //     //    EncryptedLabel encrypted_label = dealgebraize_label(
            //     //        alg_label, received_label_bit_count, params_.seal_params().plain_modulus());

            //     //    // Resize down to the effective byte count
            //     //    encrypted_label.resize(effective_label_byte_count);

            //     //    // Decrypt the label
            //     //    Label label =
            //     //        decrypt_label(encrypted_label, label_keys[item_idx], nonce_byte_count);

            //     //    // Set the label
            //     //    mr.label.set(move(label));
            //     //}

            //     // We are done with the MatchRecord, so add it to the mrs vector
            //     mrs[item_idx] = move(mr);
            // });

            //return mrs;
        }

        //vector<MatchRecord> Receiver::process_result(
        //    const vector<LabelKey> &label_keys,
        //    const IndexTranslationTable &itt,
        //    const vector<ResultPart> &result) const
        //{
        //    APSU_LOG_INFO("Processing " << result.size() << " result parts");
        //    STOPWATCH(recv_stopwatch, "Receiver::process_result");

        //    vector<MatchRecord> mrs(itt.item_count());

        //    for (auto &result_part : result) {
        //        auto this_mrs = process_result_part(label_keys, itt, result_part);
        //        if (this_mrs.size() != mrs.size()) {
        //            // Something went wrong with process_result; error is already logged
        //            continue;
        //        }

        //        // Merge the new MatchRecords with mrs
        //        seal_for_each_n(iter(mrs, this_mrs, size_t(0)), mrs.size(), [](auto &&I) {
        //            if (get<1>(I) && !get<0>(I)) {
        //                // This match needs to be merged into mrs
        //                get<0>(I) = move(get<1>(I));
        //            } else if (get<1>(I) && get<0>(I)) {
        //                // If a positive MatchRecord is already present, then something is seriously
        //                // wrong
        //                APSU_LOG_ERROR(
        //                    "Found a match for items["
        //                    << get<2>(I)
        //                    << "] but an existing match for this "
        //                       "location was already found before from a different result part");

        //                throw runtime_error(
        //                    "found a duplicate positive match; something is seriously wrong");
        //            }
        //        });
        //    }

        //    APSU_LOG_INFO(
        //        "Found " << accumulate(mrs.begin(), mrs.end(), 0, [](auto acc, auto &curr) {
        //            return acc + curr.found;
        //        }) << " matches");

        //    return mrs;
        //}

        void Receiver::process_result_worker(
            atomic<uint32_t> &package_count,
            vector<MatchRecord> &mrs,
            const IndexTranslationTable &itt,
            NetworkChannel &chl)
        {
            stringstream sw_ss;
            sw_ss << "Receiver::process_result_worker [" << this_thread::get_id() << "]";
            STOPWATCH(recv_stopwatch, sw_ss.str());

            APSU_LOG_INFO("Result worker [" << this_thread::get_id() << "]: starting");

            auto seal_context = get_seal_context();

            while (true) {
                // Return if all packages have been claimed
                uint32_t curr_package_count = package_count;
                if (curr_package_count == 0) {
                    APSU_LOG_DEBUG(
                        "Result worker [" << this_thread::get_id()
                                          << "]: all packages claimed; exiting");
                    return;
                }

                // If there has been no change to package_count, then decrement atomically
                if (!package_count.compare_exchange_strong(
                        curr_package_count, curr_package_count - 1)) {
                    continue;
                }

                // Wait for a valid ResultPart
                ResultPart result_part;
                while (!(result_part = chl.receive_result(seal_context)))
                    ;
                // PlainResultPackage plain_rp = result_part->extract(crypto_context_);
                // uint32_t idx = result_part->cache_idx;
                // uint32_t size = plain_rp.psi_result.size();
                // APSU_LOG_INFO("before copy"<<idx<<' '<<size<<' ');
                // uint32_t items_per_bundle = safe_cast<uint32_t>(params_.items_per_bundle());
                // size_t felts_per_item = safe_cast<size_t>(params_.item_params().felts_per_item);
                // int block_num = ((felts_per_item+3)/4);
                // vector<vector<oc::block > > receiver_set(block_num);

                // for(int i=0;i<items_per_bundle;i++){
                //     vector<uint64_t> rest(block_num*4,0);
                //     for(int j = 0;j<felts_per_item;j++){
                //         rest[j] = plain_rp.psi_result[i*felts_per_item+j];
                //     }
                //     for(int j=0;j<block_num*4;j+=4){
                //         uint64_t l =(uint64_t) (((rest[j+1]&0xffffffff)<<32)|(rest[j+0]&0xffffffff));
                //         uint64_t r =(uint64_t) (((rest[j+3]&0xffffffff)<<32)|(rest[j+2]&0xffffffff));
                //         receiver_set[j/4].push_back(oc::toBlock(r,l));
                //     }
                // }

                // for(int j=0;j<block_num;j++){
                //     copy(
                //     receiver_set[j].begin(),
                //     receiver_set[j].end(),
                //     psi_result_before_shuffle[j].begin()+(idx*items_per_bundle));
                // }
            // Decrypt and decode the result; the result vector will have full batch size
                     
                    PlainResultPackage plain_rp = result_part->extract(crypto_context_);
                    uint32_t items_per_bundle = safe_cast<uint32_t>(params_.items_per_bundle());
                    uint32_t bundle_idx_count = safe_cast<uint32_t>(params_.bundle_idx_count());
                    size_t felts_per_item = safe_cast<size_t>(params_.item_params().felts_per_item);
                    vector<oc::block> decrypt_res(items_per_bundle);
                    uint64_t plain_modulus=crypto_context_.seal_context()->last_context_data()->parms().plain_modulus().value();
                    // for(int i =0 ;i<plain_rp.psi_result.size();i++){
                    //     plain_rp.psi_result[i] = 124u;
                    // }
                  
                    for(uint32_t item_idx=0;item_idx<items_per_bundle;item_idx++){
                        vector<uint64_t> all_felts_one_item(felts_per_item,0);
                        for(size_t felts_idx = 0;felts_idx<felts_per_item;felts_idx++){
                            all_felts_one_item[felts_idx] = plain_rp.psi_result[item_idx*felts_per_item+felts_idx];
                        }
                        decrypt_res[item_idx]= vec_to_oc_block(all_felts_one_item,felts_per_item,plain_modulus);
               //         Block::PrintBlock(block_oc_to_std( decrypt_res[item_idx]));
                    }
                    
                    uint32_t cache_idx = result_part->cache_idx;
                    uint32_t bundle_idx = result_part->bundle_idx;

                    std::copy(
                        decrypt_res.begin(),
                        decrypt_res.end(),
                        decrypt_randoms_matrix.begin()+(cache_idx*items_per_bundle*bundle_idx_count+bundle_idx*items_per_bundle)
                    );
                // APSU_LOG_INFO("cbp_idx"<<cache_idx<<' '<<bundle_idx);
                // APSU_LOG_INFO("items_per_bundle"<<items_per_bundle);
                // APSU_LOG_INFO("bundle_idx_count"<<bundle_idx_count);
                // //APSU_LOG_INFO("bundle_idx_count"<<bundle_idx_count);
                // APSU_LOG_INFO(' '<<cache_idx*items_per_bundle*bundle_idx_count+bundle_idx*items_per_bundle);
                // Process the ResultPart to get the corresponding vector of MatchRecords
               // process_result_part( itt, result_part, chl);
                //auto this_mrs = process_result_part( itt, result_part, chl);

                // // Merge the new MatchRecords with mrs
                // seal_for_each_n(iter(mrs, this_mrs, size_t(0)), mrs.size(), [](auto &&I) {
                //     if (get<1>(I) && !get<0>(I)) {
                //         // This match needs to be merged into mrs
                //         get<0>(I) = move(get<1>(I));
                //     } else if (get<1>(I) && get<0>(I)) {
                //         // If a positive MatchRecord is already present, then something is seriously
                //         // wrong
                //         APSU_LOG_ERROR(
                //             "Result worker [" << this_thread::get_id()
                //                               << "]: found a match for items[" << get<2>(I)
                //                               << "] but an existing match for this location was "
                //                                  "already found before from a different result "
                //                                  "part");

                //         throw runtime_error(
                //             "found a duplicate positive match; something is seriously wrong");
                //     }
                // });
            }
        }
        void Receiver::ResponseOT(string conn_addr){
            all_timer.setTimePoint("response OT start");

            int numThreads = 5;
      
            oc::IOService ios;
            oc::Session recv_session=oc::Session(ios,"localhost:59999",oc::SessionMode::Client);
            std::vector<oc::Channel> recv_chls(numThreads);

            oc::PRNG prng(oc::sysRandomSeed());            
            for (int i = 0; i < numThreads; ++i)
                recv_chls[i]=recv_session.addChannel();
            std::vector<oc::IknpOtExtSender> senders(numThreads);
            APSU_LOG_INFO(sendMessages.size());
            senders[0].sendChosen(shuffleMessages, prng, recv_chls[0]);

            int recv_num = recv_chls[0].getTotalDataRecv();
            int send_num = recv_chls[0].getTotalDataSent();

         //   APSU_LOG_INFO("send_com_size ps"<<send_size/1024<<"KB");
         //   APSU_LOG_INFO("recv_com_size ps"<<receiver_size/1024<<"KB");
            APSU_LOG_INFO("OT send_com_size ps"<<send_num/1024<<"KB");
            APSU_LOG_INFO("OT recv_com_size ps"<<recv_num/1024<<"KB");
            all_timer.setTimePoint("response OT finish");

            cout<<all_timer<<endl;
            all_timer.reset();
        
            recv_session.stop();
        }
    } // namespace receiver
} // namespace apsu