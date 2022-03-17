// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_RESULTPACKAGE_APSI_NETWORK_FBS_H_
#define FLATBUFFERS_GENERATED_RESULTPACKAGE_APSI_NETWORK_FBS_H_

#include "flatbuffers/flatbuffers.h"

#include "apsi/network/ciphertext_generated.h"

namespace apsi {
namespace network {
namespace fbs {

struct ResultPackage;
struct ResultPackageBuilder;

struct ResultPackage FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef ResultPackageBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_BUNDLE_IDX = 4,
    VT_CACHE_IDX = 6,
    VT_PSI_RESULT = 8,
    VT_LABEL_BYTE_COUNT = 10,
    VT_NONCE_BYTE_COUNT = 12,
    VT_LABEL_RESULT = 14
  };
  uint32_t bundle_idx() const {
    return GetField<uint32_t>(VT_BUNDLE_IDX, 0);
  }
  uint32_t cache_idx() const {
    return GetField<uint32_t>(VT_CACHE_IDX, 0);
  }
  const apsi::network::fbs::Ciphertext *psi_result() const {
    return GetPointer<const apsi::network::fbs::Ciphertext *>(VT_PSI_RESULT);
  }
  uint32_t label_byte_count() const {
    return GetField<uint32_t>(VT_LABEL_BYTE_COUNT, 0);
  }
  uint32_t nonce_byte_count() const {
    return GetField<uint32_t>(VT_NONCE_BYTE_COUNT, 0);
  }
  const flatbuffers::Vector<flatbuffers::Offset<apsi::network::fbs::Ciphertext>> *label_result() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<apsi::network::fbs::Ciphertext>> *>(VT_LABEL_RESULT);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint32_t>(verifier, VT_BUNDLE_IDX) &&
           VerifyField<uint32_t>(verifier, VT_CACHE_IDX) &&
           VerifyOffsetRequired(verifier, VT_PSI_RESULT) &&
           verifier.VerifyTable(psi_result()) &&
           VerifyField<uint32_t>(verifier, VT_LABEL_BYTE_COUNT) &&
           VerifyField<uint32_t>(verifier, VT_NONCE_BYTE_COUNT) &&
           VerifyOffset(verifier, VT_LABEL_RESULT) &&
           verifier.VerifyVector(label_result()) &&
           verifier.VerifyVectorOfTables(label_result()) &&
           verifier.EndTable();
  }
};

struct ResultPackageBuilder {
  typedef ResultPackage Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_bundle_idx(uint32_t bundle_idx) {
    fbb_.AddElement<uint32_t>(ResultPackage::VT_BUNDLE_IDX, bundle_idx, 0);
  }
  void add_cache_idx(uint32_t cache_idx) {
    fbb_.AddElement<uint32_t>(ResultPackage::VT_CACHE_IDX, cache_idx, 0);
  }
  void add_psi_result(flatbuffers::Offset<apsi::network::fbs::Ciphertext> psi_result) {
    fbb_.AddOffset(ResultPackage::VT_PSI_RESULT, psi_result);
  }
  void add_label_byte_count(uint32_t label_byte_count) {
    fbb_.AddElement<uint32_t>(ResultPackage::VT_LABEL_BYTE_COUNT, label_byte_count, 0);
  }
  void add_nonce_byte_count(uint32_t nonce_byte_count) {
    fbb_.AddElement<uint32_t>(ResultPackage::VT_NONCE_BYTE_COUNT, nonce_byte_count, 0);
  }
  void add_label_result(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<apsi::network::fbs::Ciphertext>>> label_result) {
    fbb_.AddOffset(ResultPackage::VT_LABEL_RESULT, label_result);
  }
  explicit ResultPackageBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  flatbuffers::Offset<ResultPackage> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<ResultPackage>(end);
    fbb_.Required(o, ResultPackage::VT_PSI_RESULT);
    return o;
  }
};

inline flatbuffers::Offset<ResultPackage> CreateResultPackage(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint32_t bundle_idx = 0,
    uint32_t cache_idx = 0,
    flatbuffers::Offset<apsi::network::fbs::Ciphertext> psi_result = 0,
    uint32_t label_byte_count = 0,
    uint32_t nonce_byte_count = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<apsi::network::fbs::Ciphertext>>> label_result = 0) {
  ResultPackageBuilder builder_(_fbb);
  builder_.add_label_result(label_result);
  builder_.add_nonce_byte_count(nonce_byte_count);
  builder_.add_label_byte_count(label_byte_count);
  builder_.add_psi_result(psi_result);
  builder_.add_cache_idx(cache_idx);
  builder_.add_bundle_idx(bundle_idx);
  return builder_.Finish();
}

inline flatbuffers::Offset<ResultPackage> CreateResultPackageDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint32_t bundle_idx = 0,
    uint32_t cache_idx = 0,
    flatbuffers::Offset<apsi::network::fbs::Ciphertext> psi_result = 0,
    uint32_t label_byte_count = 0,
    uint32_t nonce_byte_count = 0,
    const std::vector<flatbuffers::Offset<apsi::network::fbs::Ciphertext>> *label_result = nullptr) {
  auto label_result__ = label_result ? _fbb.CreateVector<flatbuffers::Offset<apsi::network::fbs::Ciphertext>>(*label_result) : 0;
  return apsi::network::fbs::CreateResultPackage(
      _fbb,
      bundle_idx,
      cache_idx,
      psi_result,
      label_byte_count,
      nonce_byte_count,
      label_result__);
}

inline const apsi::network::fbs::ResultPackage *GetResultPackage(const void *buf) {
  return flatbuffers::GetRoot<apsi::network::fbs::ResultPackage>(buf);
}

inline const apsi::network::fbs::ResultPackage *GetSizePrefixedResultPackage(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<apsi::network::fbs::ResultPackage>(buf);
}

inline bool VerifyResultPackageBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<apsi::network::fbs::ResultPackage>(nullptr);
}

inline bool VerifySizePrefixedResultPackageBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<apsi::network::fbs::ResultPackage>(nullptr);
}

inline void FinishResultPackageBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<apsi::network::fbs::ResultPackage> root) {
  fbb.Finish(root);
}

inline void FinishSizePrefixedResultPackageBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<apsi::network::fbs::ResultPackage> root) {
  fbb.FinishSizePrefixed(root);
}

}  // namespace fbs
}  // namespace network
}  // namespace apsi

#endif  // FLATBUFFERS_GENERATED_RESULTPACKAGE_APSI_NETWORK_FBS_H_
