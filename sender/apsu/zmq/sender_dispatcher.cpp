// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

// STD
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <thread>

// APSU
#include "apsu/log.h"
#include "apsu/oprf/oprf_sender.h"
#include "apsu/requests.h"
#include "apsu/zmq/sender_dispatcher.h"

// SEAL
#include "seal/util/common.h"

using namespace std;
using namespace seal;
using namespace seal::util;

namespace apsu {
    using namespace network;
    using namespace oprf;

    namespace sender {
        ZMQSenderDispatcher::ZMQSenderDispatcher(shared_ptr<SenderDB> sender_db, OPRFKey oprf_key,Sender sender)
            : sender_db_(move(sender_db)), oprf_key_(move(oprf_key)), sender_()
        {
            
            
            if (!sender_db_) {
                throw invalid_argument("sender_db is not set");
            }

            // If SenderDB is not stripped, the OPRF key it holds must be equal to the provided
            // oprf_key
            if (!sender_db_->is_stripped() && oprf_key_ != sender_db->get_oprf_key()) {
                APSU_LOG_ERROR("Failed to create ZMQSenderDispatcher: SenderDB OPRF key differs "
                               "from the given OPRF key");
                throw logic_error("mismatching OPRF keys");
            }
        }
        ZMQSenderDispatcher::ZMQSenderDispatcher(shared_ptr<SenderDB> sender_db,Sender sender)
            : sender_db_(move(sender_db)), sender_()
        {
            
            
            if (!sender_db_) {
                throw invalid_argument("sender_db is not set");
            }
     
        }
        ZMQSenderDispatcher::ZMQSenderDispatcher(shared_ptr<SenderDB> sender_db)
            : sender_db_(move(sender_db))
        {
            
            
            if (!sender_db_) {
                throw invalid_argument("sender_db is not set");
            }

            try {
                oprf_key_ = sender_db_->get_oprf_key();
            } catch (const logic_error &ex) {
                APSU_LOG_ERROR("Failed to create ZMQSenderDispatcher: missing OPRF key");
                throw;
            }
        }

        void ZMQSenderDispatcher::run(const atomic<bool> &stop, int port)
        {
            ZMQSenderChannel chl;

            stringstream ss;
            ss << "tcp://*:" << port;
           
            APSU_LOG_INFO("ZMQSenderDispatcher listening on port " << port);
            chl.bind(ss.str());
         
            auto seal_context = sender_db_->get_seal_context();

            // Run until stopped
            bool logged_waiting = false;
            while (!stop) {
                unique_ptr<ZMQSenderOperation> sop;
                if (!(sop = chl.receive_network_operation(seal_context))) {
                    if (!logged_waiting) {
                        // We want to log 'Waiting' only once, even if we have to wait
                        // for several sleeps. And only once after processing a request as well.
                        logged_waiting = true;
                        APSU_LOG_INFO("Waiting for request from Receiver");
                    }

                    this_thread::sleep_for(50ms);
                    continue;
                }

                switch (sop->sop->type()) {
                    
                case SenderOperationType::sop_parms:
                    APSU_LOG_INFO("Received parameter request");
                    dispatch_parms(move(sop), chl);
                    break;

     

                case SenderOperationType::sop_query:
                    APSU_LOG_INFO("Received query");
                    dispatch_query(move(sop), chl);
                    break;
                case SenderOperationType::sop_response:
                    APSU_LOG_INFO("Received response");
                    dispatch_re(move(sop), chl);
                    break;
                default:
                    // We should never reach this point
                    //std::cout << (int)(sop->sop->type()) << endl;
                    throw runtime_error("invalid operation");
                }

                logged_waiting = false;
            }
        }

        void ZMQSenderDispatcher::dispatch_parms(
            unique_ptr<ZMQSenderOperation> sop, ZMQSenderChannel &chl)
        {
            STOPWATCH(sender_stopwatch, "ZMQSenderDispatcher::dispatch_params");

            try {
                // Extract the parameter request
                ParamsRequest params_request = to_params_request(move(sop->sop));

                sender_.RunParams(
                    params_request,
                    sender_db_,
                    chl,
                    [&sop](Channel &c, unique_ptr<SenderOperationResponse> sop_response) {
                        auto nsop_response = make_unique<ZMQSenderOperationResponse>();
                        nsop_response->sop_response = move(sop_response);
                        nsop_response->client_id = move(sop->client_id);

                        // We know for sure that the channel is a SenderChannel so use static_cast
                        static_cast<ZMQSenderChannel &>(c).send(move(nsop_response));
                    });
            } catch (const exception &ex) {
                APSU_LOG_ERROR(
                    "Sender threw an exception while processing parameter request: " << ex.what());
            }
        }

// oprf has been removed 
        void ZMQSenderDispatcher::dispatch_oprf(
            unique_ptr<ZMQSenderOperation> sop, ZMQSenderChannel &chl)
        {
            STOPWATCH(sender_stopwatch, "ZMQSenderDispatcher::dispatch_oprf");

            try {
                // Extract the OPRF request
                OPRFRequest oprf_request = to_oprf_request(move(sop->sop));

                sender_.RunOPRF(
                    oprf_request,
                    oprf_key_,
                    chl,
                    [&sop](Channel &c, unique_ptr<SenderOperationResponse> sop_response) {
                        auto nsop_response = make_unique<ZMQSenderOperationResponse>();
                        nsop_response->sop_response = move(sop_response);
                        nsop_response->client_id = move(sop->client_id);

                        // We know for sure that the channel is a SenderChannel so use static_cast
                        static_cast<ZMQSenderChannel &>(c).send(move(nsop_response));
                    });
            } catch (const exception &ex) {
                APSU_LOG_ERROR(
                    "Sender threw an exception while processing OPRF request: " << ex.what());
            }
        }

        void ZMQSenderDispatcher::dispatch_query(
            unique_ptr<ZMQSenderOperation> sop, ZMQSenderChannel &chl)
        {
            STOPWATCH(sender_stopwatch, "ZMQSenderDispatcher::dispatch_query");

            try {
                // Create the Query object
                Query query(to_query_request(move(sop->sop)), sender_db_);

                // Query will send result to client in a stream of ResultPackages (ResultParts)
                sender_.RunQuery(
                    query,
                    chl,
                    // Lambda function for sending the query response
                    [&sop](Channel &c, Response response) {
                        auto nsop_response = make_unique<ZMQSenderOperationResponse>();
                        nsop_response->sop_response = move(response);
                        nsop_response->client_id = sop->client_id;

                        // We know for sure that the channel is a SenderChannel so use static_cast
                        static_cast<ZMQSenderChannel &>(c).send(move(nsop_response));

                    },
                    // Lambda function for sending the result parts
                    [&sop](Channel &c, ResultPart rp) {
                        auto nrp = make_unique<ZMQResultPackage>();
                        nrp->rp = move(rp);
                        nrp->client_id = sop->client_id;

                        // We know for sure that the channel is a SenderChannel so use static_cast
                        static_cast<ZMQSenderChannel &>(c).send(move(nrp));
                    } 
                    );
            } catch (const exception &ex) {
                APSU_LOG_ERROR("Sender threw an exception while processing query: " << ex.what());
            }
        }
        void ZMQSenderDispatcher::dispatch_re(
            unique_ptr<ZMQSenderOperation> sop, ZMQSenderChannel& chl)
        {
            try {
                plainRequest response = to_plain_request(move(sop->sop));
                cout << "wow " << endl;
                PSIParams params_ = sender_db_->get_params();
                sender_.RunResponse(response, chl, move(params_));

            } catch (const exception &ex) {
                APSU_LOG_ERROR("Sender threw an exception while processing response: " << ex.what());
            }
        }


    } // namespace sender
} // namespace apsu
