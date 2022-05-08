#ifndef KUNLUN_POPRF_HPP_
#define KUNLUN_POPRF_HPP_

#include "../../crypto/ec_point.hpp"
#include "../../crypto/hash.hpp"
#include "../../crypto/prg.hpp"
#include "../../crypto/block.hpp"
#include "../../netio/stream_channel.hpp"



/*
** implement (permuted)-OPRF based on the DDH Assumption
*/


namespace DDHOPRF{

struct PP
{
   std::string str_reserve;   
};

// seriazlize
std::ofstream &operator<<(std::ofstream &fout, const PP &pp)
{ 
    fout << pp.str_reserve;
    return fout; 
}

// load pp from file
std::ifstream &operator>>(std::ifstream &fin, PP &pp)
{
    fin >> pp.str_reserve;
    return fin; 
}

PP Setup()
{
    PP pp; 
    pp.str_reserve = "";
    return pp; 
}

void SavePP(PP &pp, std::string pp_filename)
{
    std::ofstream fout; 
    fout.open(pp_filename, std::ios::binary); 
    if(!fout)
    {
        std::cerr << pp_filename << " open error" << std::endl;
        exit(1); 
    }
    fout << pp; 
    fout.close(); 
}

void FetchPP(PP &pp, std::string pp_filename)
{
    std::ifstream fin; 
    fin.open(pp_filename, std::ios::binary); 
    if(!fin)
    {
        std::cerr << pp_filename << " open error" << std::endl;
        exit(1); 
    }
    fin >> pp; 
    fin.close(); 
}

// the default permutation_map should be an identity mapping
BigInt Server(NetIO &io, PP &pp, std::vector<uint64_t> permutation_map, size_t LEN)
{
    PrintSplitLine('-'); 
    auto start_time = std::chrono::steady_clock::now(); 

    BigInt k = GenRandomBigIntLessThan(order); // pick a key k

    std::vector<ECPoint> vec_mask_X(LEN); 
    io.ReceiveECPoints(vec_mask_X.data(), LEN);

    std::vector<ECPoint> vec_Fk_mask_X(LEN); 
    #pragma omp parallel for
    for(auto i = 0; i < LEN; i++){ 
        vec_Fk_mask_X[permutation_map[i]] = vec_mask_X[i].ThreadSafeMul(k); 
    }

    io.SendECPoints(vec_Fk_mask_X.data(), LEN);

    std::cout <<"DDH-based (permuted)-OPRF [step 2]: Server ===> F_k(mask_x_i) ===> Client";
    #ifdef POINT_COMPRESSED
        std::cout << " [" << (double)POINT_COMPRESSED_BYTE_LEN*LEN/(1024*1024) << " MB]" << std::endl;
    #else
        std::cout << " [" << (double)POINT_BYTE_LEN*LEN/(1024*1024) << " MB]" << std::endl;
    #endif


    auto end_time = std::chrono::steady_clock::now(); 
    auto running_time = end_time - start_time;
    std::cout << "DDH-based (permuted)-OPRF: Server side takes time = " 
              << std::chrono::duration <double, std::milli> (running_time).count() << " ms" << std::endl;
    PrintSplitLine('-'); 

    return k; 
}

void Client(NetIO &io, PP &pp, std::vector<block> &vec_X, size_t LEN) 
{    
    PrintSplitLine('-'); 

    auto start_time = std::chrono::steady_clock::now(); 

    BigInt r = GenRandomBigIntLessThan(order); // pick a mask

    std::vector<ECPoint> vec_mask_X(LEN); 
    #pragma omp parallel for
    for(auto i = 0; i < LEN; i++){
        vec_mask_X[i] = Hash::ThreadSafeBlockToECPoint(vec_X[i]).ThreadSafeMul(r); // H(x_i)^r
    } 
    io.SendECPoints(vec_mask_X.data(), LEN);
    
    std::cout <<"DDH-based (permuted)-OPRF [step 1]: Client ===> mask_x_i ===> Server"; 
    #ifdef POINT_COMPRESSED
        std::cout << " [" << (double)POINT_COMPRESSED_BYTE_LEN*LEN/(1024*1024) << " MB]" << std::endl;
    #else
        std::cout << " [" << (double)POINT_BYTE_LEN*LEN/(1024*1024) << " MB]" << std::endl;
    #endif

    // first receive incoming data
    std::vector<ECPoint> vec_Fk_mask_X(LEN);
    io.ReceiveECPoints(vec_Fk_mask_X.data(), LEN); // receive F_k(mask_x_i) from Server


    BigInt r_inverse = r.ModInverse(order); 
    std::vector<ECPoint> vec_Fk_X(LEN);
    #pragma omp parallel for
    for(auto i = 0; i < LEN; i++){
        vec_Fk_X[i] = vec_Fk_mask_X[i].ThreadSafeMul(r_inverse); 
    }

    auto end_time = std::chrono::steady_clock::now(); 
    auto running_time = end_time - start_time;
    std::cout << "DDH-based (permuted)-OPRF: Client side takes time = " 
              << std::chrono::duration <double, std::milli> (running_time).count() << " ms" << std::endl;

        
    PrintSplitLine('-'); 
}



}
#endif
