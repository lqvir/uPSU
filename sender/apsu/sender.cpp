// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

// STD
#include <future>
#include <sstream>
#include <fstream>
// APSU
#include "apsu/crypto_context.h"
#include "apsu/log.h"
#include "apsu/network/channel.h"
#include "apsu/network/result_package.h"
#include "apsu/psi_params.h"
#include "apsu/seal_object.h"
#include "apsu/sender.h"
#include "apsu/thread_pool_mgr.h"
#include "apsu/util/stopwatch.h"
#include "apsu/util/utils.h"

// SEAL
#include "seal/evaluator.h"
#include "seal/modulus.h"
#include "seal/util/common.h"

#include "kunlun/mpc/oprf/mp_oprf.hpp"


using namespace std;
using namespace seal;
using namespace seal::util;

namespace apsu {
    using namespace util;
    using namespace oprf;
    using namespace network;

    namespace sender {

        namespace {
            vector<oc::block> random_matrix;
            std::vector<std::string> mpoprf_out;
            template <typename T>
            bool has_n_zeros(T *ptr, size_t count)
            {
                return all_of(ptr, ptr + count, [](auto a) { return a == T(0); });
            }
            inline block block_oc_to_std(oc::block in){
                return Block::MakeBlock(in.as<uint64_t>()[1],in.as<uint64_t>()[0]);
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
                // auto temp = oc::toBlock(higher,lower);
                // auto h = temp.as<uint64_t>()[0];
                // auto l = temp.as<uint64_t>()[1];
                // cout<<higher<<endl<<lower<<endl<<h<<endl<<l<<endl;
                // Block::PrintBlock(Block::MakeBlock(higher,lower));
                // Block::PrintBlock(block_oc_to_std(temp));
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
           

                return Block::MakeBlock(higher,lower);
            }

            // #define block_oc_to_std(a) (Block::MakeBlock(a.as<uint64_t>()[1],a.as<uint64_t>()[0]))
            oc::Timer all_timer;
        } // namespace
       
        void Sender::RunParams(
            const ParamsRequest &params_request,
            shared_ptr<SenderDB> sender_db,
            network::Channel &chl,
            function<void(Channel &, Response)> send_fun)
        {
            STOPWATCH(sender_stopwatch, "Sender::RunParams");
            all_timer.setTimePoint("RunParames start");
            if (!params_request) {
                APSU_LOG_ERROR("Failed to process parameter request: request is invalid");
                throw invalid_argument("request is invalid");
            }

            // Check that the database is set
            if (!sender_db) {
                throw logic_error("SenderDB is not set");
            }

            APSU_LOG_INFO("Start processing parameter request");
         
   
            ParamsResponse response_params = make_unique<ParamsResponse::element_type>();
            response_params->params = make_unique<PSIParams>(sender_db->get_params());

            try {
                send_fun(chl, move(response_params));
            } catch (const exception &ex) {
                APSU_LOG_ERROR(
                    "Failed to send response to parameter request; function threw an exception: "
                    << ex.what());
                throw;
            }

            APSU_LOG_INFO("Finished processing parameter request");
            all_timer.setTimePoint("RunParames finish");
        }

        void Sender::RunOPRF(
            const OPRFRequest &oprf_request,
            OPRFKey key,
            network::Channel &chl,
            function<void(Channel &, Response)> send_fun)
        {
            STOPWATCH(sender_stopwatch, "Sender::RunOPRF");

            if (!oprf_request) {
                APSU_LOG_ERROR("Failed to process OPRF request: request is invalid");
                throw invalid_argument("request is invalid");
            }

            APSU_LOG_INFO(
                "Start processing OPRF request for " << oprf_request->data.size() / oprf_query_size
                                                     << " items");

            // OPRF response has the same size as the OPRF query
            OPRFResponse response_oprf = make_unique<OPRFResponse::element_type>();
            try {
                response_oprf->data = OPRFSender::ProcessQueries(oprf_request->data, key);
            } catch (const exception &ex) {
                // Something was wrong with the OPRF request. This can mean malicious
                // data being sent to the sender in an attempt to extract OPRF key.
                // Best not to respond anything.
                APSU_LOG_ERROR("Processing OPRF request threw an exception: " << ex.what());
                return;
            }

            try {
                send_fun(chl, move(response_oprf));
            } catch (const exception &ex) {
                APSU_LOG_ERROR(
                    "Failed to send response to OPRF request; function threw an exception: "
                    << ex.what());
                throw;
            }

            APSU_LOG_INFO("Finished processing OPRF request");
        }

        void Sender::RunQuery(
            const Query &query,
            Channel &chl,
            function<void(Channel &, Response)> send_fun,
            function<void(Channel &, ResultPart)> send_rp_fun
           )
        {
            all_timer.setTimePoint("RunQuery start");
            random_map.clear();
            random_after_permute_map.clear();
            random_plain_list.clear();
            
            
            if (!query) {
                APSU_LOG_ERROR("Failed to process query request: query is invalid");
                throw invalid_argument("query is invalid");
            }

            // We use a custom SEAL memory that is freed after the query is done
            auto pool = MemoryManager::GetPool(mm_force_new);

            ThreadPoolMgr tpm;

            // Acquire read lock on SenderDB
            auto sender_db = query.sender_db();
            auto sender_db_lock = sender_db->get_reader_lock();

            STOPWATCH(sender_stopwatch, "Sender::RunQuery");
            APSU_LOG_INFO(
                "Start processing query request on database with " << sender_db->get_item_count()
                                                                   << " items");

            // Copy over the CryptoContext from SenderDB; set the Evaluator for this local instance.
            // Relinearization keys may not have been included in the query. In that case
            // query.relin_keys() simply holds an empty seal::RelinKeys instance. There is no
            // problem with the below call to CryptoContext::set_evaluator.
            CryptoContext crypto_context(sender_db->get_crypto_context());
            crypto_context.set_evaluator(query.relin_keys());

            // Get the PSIParams
            PSIParams params(sender_db->get_params());

            uint32_t bundle_idx_count = safe_cast<uint32_t>(params.bundle_idx_count());
            uint32_t max_items_per_bin = safe_cast<uint32_t>(params.table_params().max_items_per_bin);

            
            
            
            size_t max_bin_bundle_conut_alpha = 0;
            std::vector<size_t> cache_cnt_per_bundle;
            for (size_t bundle_idx = 0; bundle_idx < bundle_idx_count; bundle_idx++) {
                cache_cnt_per_bundle.emplace_back(sender_db->get_bin_bundle_count(static_cast<uint32_t>(bundle_idx)));
                max_bin_bundle_conut_alpha = std::max(max_bin_bundle_conut_alpha,
                    cache_cnt_per_bundle[bundle_idx]);
            }
            
            
            
            // Extract the PowersDag
            PowersDag pd = query.pd();

            // The query response only tells how many ResultPackages to expect; send this first
            uint32_t package_count = safe_cast<uint32_t>(sender_db->get_bin_bundle_count());
            QueryResponse response_query = make_unique<QueryResponse::element_type>();
            response_query->package_count = package_count;
            response_query->alpha_max_cache_count = max_bin_bundle_conut_alpha;
            APSU_LOG_INFO(package_count);
            item_cnt = bundle_idx_count * safe_cast<uint32_t>(params.items_per_bundle());
            APSU_LOG_INFO(item_cnt);
            try {
                send_fun(chl, move(response_query));
            } catch (const exception &ex) {
                APSU_LOG_ERROR(
                    "Failed to send response to query request; function threw an exception: "
                    << ex.what());
                throw;
            }
            // generate random number
            {
                all_timer.setTimePoint("random gen start");
                std::vector<uint64_t> random_num;
                prng_seed_type newseed;
                random_bytes(reinterpret_cast<seal_byte *>(newseed.data()), prng_seed_byte_count);
                UniformRandomGeneratorInfo myGEN(prng_type::blake2xb, newseed);
                std::shared_ptr<UniformRandomGenerator> myprng = myGEN.make_prng();
                auto context_data = crypto_context.seal_context()->last_context_data();
                // cout << "mod q" << endl;
                // random mod q
                //cout << context_data->parms().coeff_modulus().back().value() << endl;
                uint64_t plain_modulus = context_data->parms().plain_modulus().value();
                auto encoder = crypto_context.encoder();
                int slot_count = encoder->slot_count();

                size_t felts_per_item = safe_cast<size_t>(params.item_params().felts_per_item);
                size_t items_per_bundle = safe_cast<size_t>(params.items_per_bundle());
 

                for(size_t cache_idx = 0;cache_idx < max_bin_bundle_conut_alpha;cache_idx++)
                 {
                    for (size_t bundle_idx = 0; bundle_idx < bundle_idx_count; bundle_idx++){
                        // judge need to padding the cache
                        bool padding_to_max_cache_size=(cache_idx>=cache_cnt_per_bundle[bundle_idx]);
                //        APSU_LOG_INFO("error"<<cache_idx<<' '<<cache_cnt_per_bundle[bundle_idx]);
                        if(padding_to_max_cache_size){  
                            
                            for(size_t i = 0;i< items_per_bundle;i++)
                                random_matrix.emplace_back(oc::AllOneBlock);
                            continue;
                        }
                        
                        Plaintext random_plain(pool);
                        random_num.clear();
                        for (int i = 0; i < slot_count; i++)
                        {
                            random_num.emplace_back(myprng->generate() % plain_modulus);
                            //random_num.emplace_back(0);
                                    //random_num.push_back(i % small_q);
                                    //random_num.push_back(0);
                        }
                        // gsl::span<uint64_t> random_mem = { random_num.data(), random_num.size() };
                        for(size_t i=0;i<items_per_bundle;i++){
                            vector<uint64_t> rest(felts_per_item,0);
                            for(size_t j = 0;j<felts_per_item;j++){
                                rest[j] = random_num[i*felts_per_item+j];
                            }
                            //random_map_block[bundle_idx].emplace_back(vec_to_std_block(move(rest),felts_per_item,plain_modulus));
                            random_matrix.emplace_back(vec_to_oc_block(rest,felts_per_item,plain_modulus));
                            
                            
                        }

                      //  APSU_LOG_INFO("cbp_idx"<<cache_idx<<' '<<bundle_idx<<' '<<random_plain_list.size());

                        encoder->encode(random_num, random_plain);
                        random_plain_list.emplace_back(random_plain);
                        random_map.emplace_back(random_num);
                      
                        // for(auto x :  random_map[0])
                        //     std::cout<<x<<endl;
                    }
                                  
                }
     

               
                // for(int i=0;i<package_count;i++){
                //     for(int j=0;j<receiver_set_size1;i++){
                //         auto temp = receiver_share[i][j];
                //         random_after_permute_map[i].push_back(temp.as<uint32_t>()[0]);
                //         random_after_permute_map[i].push_back(temp.as<uint32_t>()[1]);
                //         random_after_permute_map[i].push_back(temp.as<uint32_t>()[2]);
                //         random_after_permute_map[i].push_back(temp.as<uint32_t>()[3]);
                //     }
                //     for(int j=0;j<receiver_set_size2;j++){
                //         auto temp = receiver_share[i][receiver_set_size1];
                //         random_after_permute_map[i].push_back(temp.as<uint32_t>()[j]);
                //     }
                // }
              

            }


            // For each bundle index i, we need a vector of powers of the query Qᵢ. We need powers
            // all the way up to Qᵢ^max_items_per_bin. We don't store the zeroth power. If
            // Paterson-Stockmeyer is used, then only a subset of the powers will be populated.
            vector<CiphertextPowers> all_powers(bundle_idx_count);
            all_timer.setTimePoint("compute power start");

            // Initialize powers
            for (CiphertextPowers &powers : all_powers) {
                // The + 1 is because we index by power. The 0th power is a dummy value. I promise
                // this makes things easier to read.
                size_t powers_size = static_cast<size_t>(max_items_per_bin) + 1;
                powers.reserve(powers_size);
                for (size_t i = 0; i < powers_size; i++) {
                    powers.emplace_back(pool);
                }
            }

            // Load inputs provided in the query
            for (auto &q : query.data()) {
                // The exponent of all the query powers we're about to iterate through
                size_t exponent = static_cast<size_t>(q.first);

                // Load Qᵢᵉ for all bundle indices i, where e is the exponent specified above
                for (size_t bundle_idx = 0; bundle_idx < all_powers.size(); bundle_idx++) {
                    // Load input^power to all_powers[bundle_idx][exponent]
                    APSU_LOG_DEBUG(
                        "Extracting query ciphertext power " << exponent << " for bundle index "
                                                             << bundle_idx);
                    all_powers[bundle_idx][exponent] = move(q.second[bundle_idx]);
                }
            }

            // Compute query powers for the bundle indexes
            for (size_t bundle_idx = 0; bundle_idx < bundle_idx_count; bundle_idx++) {
                ComputePowers(
                    sender_db,
                    crypto_context,
                    all_powers,
                    pd,
                    static_cast<uint32_t>(bundle_idx),
                    pool);
            }
            all_timer.setTimePoint("compute power finished");

            APSU_LOG_DEBUG("Finished computing powers for all bundle indices");
            APSU_LOG_DEBUG("Start processing bin bundle caches");

            vector<future<void>> futures;
            for (size_t bundle_idx = 0; bundle_idx < bundle_idx_count; bundle_idx++) {
                auto bundle_caches = sender_db->get_cache_at(static_cast<uint32_t>(bundle_idx));
                size_t cache_idx = 0;
               // APSU_LOG_INFO(cache_idx);
                for (auto &cache : bundle_caches) {
                    pack_cnt++;
                    size_t pack_idx = bundle_idx+cache_idx*bundle_idx_count;
                    futures.push_back(tpm.thread_pool().enqueue([&, bundle_idx, cache,cache_idx,pack_idx]() {
                        ProcessBinBundleCache(
                            sender_db,
                            crypto_context,
                            cache,
                            all_powers,
                            chl,
                            send_rp_fun,
                            static_cast<uint32_t>(bundle_idx),
                            query.compr_mode(),
                            pool,
                            cache_idx,
                            pack_idx
                            );
                    }));
                    cache_idx++;
                }
            }

            // Wait until all bin bundle caches have been processed
            for (auto &f : futures) {
                f.get();
            }

            vector<block> mpoprf_in;
           size_t shuffle_size = random_matrix.size();
            // permute and share on matrix 
            {
                
                
                vector<oc::block> receiver_share;

                
                APSU_LOG_INFO("size"<<shuffle_size<<"==============");
               
                all_timer.setTimePoint("random gen finish");
                int numThreads=2;
                osuCrypto::IOService ios;
                
                oc::Session send_session=oc::Session(ios,"localhost:59999",oc::SessionMode::Server);
                std::vector<oc::Channel> send_chls(numThreads);
                for(int i=0;i<numThreads;i++)
                    send_chls[i]=send_session.addChannel();

                OSNReceiver osn;
                osn.init(shuffle_size,1);
                oc::Timer timer;
	            osn.setTimer(timer);

	            timer.setTimePoint("set -> block_set");
                all_timer.setTimePoint("set -> block_set");
                
                APSU_LOG_INFO("per change"<<random_matrix.size());
                all_timer.setTimePoint("before run_osn");
	            timer.setTimePoint("before run_osn");

                receiver_share =osn.run_osn(random_matrix,send_chls);
                
                timer.setTimePoint("after run_osn");
                all_timer.setTimePoint("after run_osn");
// block change   timer.setTimePoint("block_set -> set");
                APSU_LOG_INFO("receiver"<<timer);
                all_timer.setTimePoint("block_set -> set");
                send_size=0,receiver_size =0;
                for(auto x: send_chls){
                    send_size+=x.getTotalDataSent();
                    receiver_size+=x.getTotalDataRecv();
                }
                send_session.stop();
                
                for(auto x: receiver_share){
                    mpoprf_in.emplace_back(block_oc_to_std(x));
                }
                size_t padding = 128-mpoprf_in.size()%128;
                for(int i = 0;i<padding;i++)
                    mpoprf_in.emplace_back(Block::zero_block);
            }
            // mp-oprf 
           // Block::PrintBlocks(mpoprf_in);
            std::vector<std::string> mpoprf_recv;
            {
             
                Global_Setup(); 
        	    Context_Initialize(); 
                ECGroup_Initialize(NID_X9_62_prime256v1); 
                NetIO server("server", "", 59999);
                size_t set_size = mpoprf_in.size();
                size_t log_set_size=int(log2(set_size)+1);

                std::cout<<set_size<<std::endl;
                std::cout<<log_set_size<<std::endl;
                std::string pp_filename = "MPOPRF.pp"; 
                MPOPRF::PP pp; 
                
                pp = MPOPRF::Setup(log_set_size);
                // if(!FileExist(pp_filename)){
                //     pp = MPOPRF::Setup(log_set_size); // 40 is the statistical parameter
                //     MPOPRF::SavePP(pp, pp_filename); 
                // }
                // else{
                //     MPOPRF::FetchPP(pp, pp_filename); 
                // }
                
                
              //  NetIO server("server", "", 59998);

                mpoprf_out = MPOPRF::Receive(server,pp,mpoprf_in);
               // APSU_LOG_INFO(pp.H2_OUTPUT_LEN);
                mpoprf_recv.assign(shuffle_size,std::string(pp.H2_OUTPUT_LEN,'0'));
               
                for(int i = 0;i<shuffle_size;i++){
                    //  APSU_LOG_INFO(i);
                    server.ReceiveString(mpoprf_recv[i]);
                    //  APSU_LOG_INFO(mpoprf_recv[i]);

                }
                ECGroup_Finalize(); 
                Context_Finalize();
            }
            
            // generate ans;
            {

            for(size_t cache_idx = 0;cache_idx<max_bin_bundle_conut_alpha;cache_idx++){
                for(size_t item_idx = 0;item_idx<item_cnt;item_idx++){
                    // cout<<mpoprf_out[cache_idx*item_cnt+item_idx].length()<<endl;
                    // cout<<mpoprf_recv[cache_idx*item_cnt+item_idx].length()<<endl;

                    if(mpoprf_out[cache_idx*item_cnt+item_idx].compare(mpoprf_recv[cache_idx*item_cnt+item_idx])==0) ans.emplace_back(item_idx);
                }
            }
            APSU_LOG_INFO("ans size"<<ans.size());
            // for(auto x: ans)
            //     cout<<x<<endl;
            }

            cout<<"pack_cnt"<<pack_cnt<<endl;
            all_timer.setTimePoint("ProcessBinBundleCache finished");
            APSU_LOG_INFO("Finished processing query request");
            RunOT();
        }

        void Sender::ComputePowers(
            const shared_ptr<SenderDB> &sender_db,
            const CryptoContext &crypto_context,
            vector<CiphertextPowers> &all_powers,
            const PowersDag &pd,
            uint32_t bundle_idx,
            MemoryPoolHandle &pool)
        {
            STOPWATCH(sender_stopwatch, "Sender::ComputePowers");
            auto bundle_caches = sender_db->get_cache_at(bundle_idx);
            if (!bundle_caches.size()) {
                return;
            }

            // Compute all powers of the query
            APSU_LOG_DEBUG("Computing all query ciphertext powers for bundle index " << bundle_idx);

            auto evaluator = crypto_context.evaluator();
            auto relin_keys = crypto_context.relin_keys();

            CiphertextPowers &powers_at_this_bundle_idx = all_powers[bundle_idx];
            bool relinearize = crypto_context.seal_context()->using_keyswitching();
            pd.parallel_apply([&](const PowersDag::PowersNode &node) {
                if (!node.is_source()) {
                    auto parents = node.parents;
                    Ciphertext prod(pool);
                    if (parents.first == parents.second) {
                        evaluator->square(powers_at_this_bundle_idx[parents.first], prod, pool);
                    } else {
                        evaluator->multiply(
                            powers_at_this_bundle_idx[parents.first],
                            powers_at_this_bundle_idx[parents.second],
                            prod,
                            pool);
                    }
                    if (relinearize) {
                        evaluator->relinearize_inplace(prod, *relin_keys, pool);
                    }
                    powers_at_this_bundle_idx[node.power] = move(prod);
                }
            });

            // Now that all powers of the ciphertext have been computed, we need to transform them
            // to NTT form. This will substantially improve the polynomial evaluation,
            // because the plaintext polynomials are already in NTT transformed form, and the
            // ciphertexts are used repeatedly for each bin bundle at this index. This computation
            // is separate from the graph processing above, because the multiplications must all be
            // done before transforming to NTT form. We omit the first ciphertext in the vector,
            // because it corresponds to the zeroth power of the query and is included only for
            // convenience of the indexing; the ciphertext is actually not set or valid for use.

            ThreadPoolMgr tpm;

            // After computing all powers we will modulus switch down to parameters that one more
            // level for low powers than for high powers; same choice must be used when encoding/NTT
            // transforming the SenderDB data.
            auto high_powers_parms_id =
                get_parms_id_for_chain_idx(*crypto_context.seal_context(), 1);
            auto low_powers_parms_id =
                get_parms_id_for_chain_idx(*crypto_context.seal_context(), 2);

            uint32_t ps_low_degree = sender_db->get_params().query_params().ps_low_degree;

            vector<future<void>> futures;
            for (uint32_t power : pd.target_powers()) {
                futures.push_back(tpm.thread_pool().enqueue([&, power]() {
                    if (!ps_low_degree) {
                        // Only one ciphertext-plaintext multiplication is needed after this
                        evaluator->mod_switch_to_inplace(
                            powers_at_this_bundle_idx[power], high_powers_parms_id, pool);

                        // All powers must be in NTT form
                        evaluator->transform_to_ntt_inplace(powers_at_this_bundle_idx[power]);
                    } else {
                        if (power <= ps_low_degree) {
                            // Low powers must be at a higher level than high powers
                            evaluator->mod_switch_to_inplace(
                                powers_at_this_bundle_idx[power], low_powers_parms_id, pool);

                            // Low powers must be in NTT form
                            evaluator->transform_to_ntt_inplace(powers_at_this_bundle_idx[power]);
                        } else {
                            // High powers are only modulus switched
                            evaluator->mod_switch_to_inplace(
                                powers_at_this_bundle_idx[power], high_powers_parms_id, pool);
                        }
                    }
                }));
            }

            for (auto &f : futures) {
                f.get();
            }
        }

        void Sender::ProcessBinBundleCache(
            const shared_ptr<SenderDB> &sender_db,
            const CryptoContext &crypto_context,
            reference_wrapper<const BinBundleCache> cache,
            vector<CiphertextPowers> &all_powers,
            Channel &chl,
            function<void(Channel &, ResultPart)> send_rp_fun,
            uint32_t bundle_idx,
            compr_mode_type compr_mode,
            MemoryPoolHandle &pool,
            uint32_t cache_idx,
            uint32_t pack_idx
            )
        {
            STOPWATCH(sender_stopwatch, "Sender::ProcessBinBundleCache");

            // Package for the result data
            auto rp = make_unique<ResultPackage>();
            rp->compr_mode = compr_mode;
            rp->cache_idx = cache_idx;
            rp->bundle_idx = bundle_idx;
            rp->nonce_byte_count = safe_cast<uint32_t>(sender_db->get_nonce_byte_count());
            rp->label_byte_count = safe_cast<uint32_t>(sender_db->get_label_byte_count());


            
            // Compute the matching result and move to rp
            const BatchedPlaintextPolyn &matching_polyn = cache.get().batched_matching_polyn;
            //random_plain.set_zero();
            // Determine if we use Paterson-Stockmeyer or not
            uint32_t ps_low_degree = sender_db->get_params().query_params().ps_low_degree;
            uint32_t degree = safe_cast<uint32_t>(matching_polyn.batched_coeffs.size()) - 1;
            bool using_ps = (ps_low_degree > 1) && (ps_low_degree < degree);
            if (using_ps) {
                rp->psi_result = matching_polyn.eval_patstock(
                    crypto_context, all_powers[bundle_idx], safe_cast<size_t>(ps_low_degree), pool,random_plain_list[pack_idx]);
            } else {
                rp->psi_result = matching_polyn.eval(all_powers[bundle_idx], pool, random_plain_list[pack_idx]);
            }
            // random_plain.set_zero();


            // Send this result part
            try {
                send_rp_fun(chl, move(rp));
            } catch (const exception &ex) {
                APSU_LOG_ERROR(
                    "Failed to send result part; function threw an exception: " << ex.what());
                throw;
            }
        }

        void Sender::RunResponse(
            const plainRequest &plain_request, network::Channel &chl,const PSIParams &params_)
        {
      
      /*      for (auto i : params_request->psi_result) {
                cout << i << endl;
            }*/

            // To be atomic counter
          

            all_timer.setTimePoint("RunResponse start");

            size_t felts_per_item = safe_cast<size_t>(params_.item_params().felts_per_item);
            size_t items_per_bundle = safe_cast<size_t>(params_.items_per_bundle());
            size_t bundle_start =
                mul_safe(safe_cast<size_t>(plain_request->bundle_idx), items_per_bundle);
            ;
            ans.clear();
            item_cnt = plain_request->psi_result.size();
            APSU_LOG_INFO("iten_cnt"<<item_cnt);
            for (int i = 0; i < item_cnt; i++) {
           /*     if (plain_request->psi_result[i] == random_mem[i]) {
                    cout << "success" << endl;

                }*/
                plain_request->psi_result[i] ^= random_after_permute_map[i];   
               // cout << (bool)plain_request->psi_result[i];
            }
           item_cnt /= felts_per_item;
            StrideIter<const uint64_t *> plain_rp_iter(
                plain_request->psi_result.data(), felts_per_item);
               seal_for_each_n(iter(plain_rp_iter, size_t(0)), item_cnt, [&](auto &&I) {
                // Find felts_per_item consecutive zeros
               bool match = has_n_zeros(get<0>(I).ptr(), felts_per_item);
                if (!match) {
                    return;
                }

                // Compute the cuckoo table index for this item. Then find the corresponding index
                // in the input items vector so we know where to place the result.
                size_t table_idx = add_safe(get<1>(I), bundle_start);
             
              //  cout  << " " <<"match" << table_idx << endl;
               // ans.push_back(table_idx);
                APSU_LOG_INFO(ans.size());
            });
            all_timer.setTimePoint("RunResponse finish");
             cout<<all_timer<<endl;
            //RunOT();

        }

        void Sender::RunOT(){
            all_timer.setTimePoint("RunOT start");

            int numThreads = 5;
            osuCrypto::IOService ios;
            oc::Session send_session=oc::Session(ios,"localhost:59999",oc::SessionMode::Server);
            std::vector<oc::Channel> send_chls(numThreads);
            
            osuCrypto::PRNG prng(osuCrypto::sysRandomSeed());
            
            for (int i = 0; i < numThreads; ++i)
                send_chls[i]=send_session.addChannel();
            std::vector<osuCrypto::IknpOtExtReceiver> receivers(numThreads);
            
            // osuCrypto::DefaultBaseOT base;
            // std::array<std::array<osuCrypto::block, 2>, 128> baseMsg;
            // base.send(baseMsg, prng, chls[0], numThreads);
            // receivers[0].setBaseOts(baseMsg, prng, chls[0]);
            cout<<"++++++++++++++"<<endl;
            cout<<hex<<item_cnt<<endl;
            osuCrypto::BitVector choices(item_cnt);
            APSU_LOG_INFO(ans.size());
            for(auto i : ans){
                choices[i] = 1;
            }
            //  for (auto i = 1; i < numThreads; ++i){
            //     receivers[i] = receivers[0].splitBase();
            // }
            
            std::vector<osuCrypto::block> messages(item_cnt);
            
            receivers[0].receiveChosen(choices, messages, prng, send_chls[0]);
            std::ofstream fout;
            fout.open("union.csv",std::ofstream::out);
            for(auto i:messages){

                if(i == oc::ZeroBlock) continue;
               
                stringstream ss;
                ss<<i.as<char>().data();
                auto x = ss.str();
                for(int i = 0;i<16;i++)
                    fout<<x[i];
                fout<<endl;
                //fout<<ss.str().substr(0,15)<<endl;
            }
            fout.close();
            all_timer.setTimePoint("RunOT finish");
            cout<<all_timer<<endl;
            APSU_LOG_INFO("send_com_size ps"<<send_size/1024<<"KB");
            APSU_LOG_INFO("recv_com_size ps"<<receiver_size/1024<<"KB");
            APSU_LOG_INFO("OT send_com_size ps"<<send_chls[0].getTotalDataSent()/1024<<"KB");
            APSU_LOG_INFO("OT recv_com_size ps"<<send_chls[0].getTotalDataRecv()/1024<<"KB");
            all_timer.reset();
            send_chls.clear();
            send_session.stop();
            cout<<1<<endl;
        }

    } // namespace sender
} // namespace apsu
