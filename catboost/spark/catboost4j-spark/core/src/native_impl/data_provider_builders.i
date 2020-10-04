%{
#include <catboost/spark/catboost4j-spark/core/src/native_impl/data_provider_builders.h>
#include <catboost/libs/data/data_provider_builders.h>
#include <catboost/libs/data/quantized_features_info.h>
#include <catboost/libs/data/unaligned_mem.h>
#include <catboost/libs/data/visitor.h>

#include <util/generic/fwd.h>
#include <util/generic/array_ref.h>
#include <util/generic/cast.h>
#include <util/generic/yexception.h>
%}

%include <enums.swg>

%include "data_provider.i"
%include "maybe_owning_array_holder.i"
%include "intrusive_ptr.i"
%include "tvector.i"

%include <bindings/swiglib/stroka.swg>


%template(TVector_TMaybeOwningConstArrayHolder_float) TVector<NCB::TMaybeOwningConstArrayHolder<float>>;


%{

template <class TDst, class TSrc>
NCB::TUnalignedArrayBuf<TDst> AsUnalignedBuf(TConstArrayRef<TSrc> data) {
    return NCB::TUnalignedArrayBuf<TDst>((const TDst*)data.data(), data.size() * sizeof(TSrc));
}

%}


namespace NCB {

    %javaconst(1);
    enum class EDatasetVisitorType {
        RawObjectsOrder,
        RawFeaturesOrder,
        QuantizedFeatures
    };

    // Adapt for SWIG + JVM
    class IQuantizedFeaturesDataVisitor {
    public:
        %extend {
            void Start(
                const TDataMetaInfo& metaInfo,
                i32 objectCount,
                const NCB::TQuantizedFeaturesInfo& quantizedFeaturesInfo
            ) throw (yexception) {
                self->Start(
                    metaInfo,
                    SafeIntegerCast<ui32>(objectCount),
                    NCB::EObjectsOrder::Undefined,
                    TVector<TIntrusivePtr<NCB::IResourceHolder>>(),
                    NCB::GetPoolQuantizationSchema(quantizedFeaturesInfo, metaInfo.ClassLabels),
                    /*wholeColumns*/ true
                );
            }

            // TCommonObjectsData

            void AddGroupId(TConstArrayRef<i64> groupIdData) throw (yexception) {
                self->AddGroupIdPart(0, AsUnalignedBuf<TGroupId>(groupIdData));
            }

            void AddSubgroupId(TConstArrayRef<i32> subgroupIdData) throw (yexception) {
                self->AddSubgroupIdPart(0, AsUnalignedBuf<TSubgroupId>(subgroupIdData));
            }

            void AddTimestamp(TConstArrayRef<i64> timestampData) throw (yexception) {
                self->AddTimestampPart(0, AsUnalignedBuf<ui64>(timestampData));
            }


            // TQuantizedObjectsData

            void AddFloatFeature(
                i32 flatFeatureIdx,
                i32 objectCount,
                i8 bitsPerDocumentFeature,
                TVector<i64>* featureDataBuffer // moved into per-object data size depends on BitsPerKey
            ) throw (yexception) {
                size_t sizeInBytes =
                    SafeIntegerCast<size_t>(objectCount)
                    * CeilDiv<size_t>(bitsPerDocumentFeature, CHAR_BIT);

                auto dataArray = TConstArrayRef<ui8>((ui8*)featureDataBuffer->data(), sizeInBytes);
                self->AddFloatFeaturePart(
                    SafeIntegerCast<ui32>(flatFeatureIdx),
                    0,
                    SafeIntegerCast<ui8>(bitsPerDocumentFeature),
                    NCB::TMaybeOwningConstArrayHolder<ui8>::CreateOwning(
                        dataArray,
                        MakeIntrusive<NCB::TVectorHolder<i64>>(std::move(*featureDataBuffer))
                    )
                );
            }

            void AddCatFeature(
                i32 flatFeatureIdx,
                i32 objectCount,
                i8 bitsPerDocumentFeature,
                TVector<i64>* featureDataBuffer // moved into per-object data size depends on BitsPerKey
            ) throw (yexception) {
                size_t sizeInBytes =
                    SafeIntegerCast<size_t>(objectCount)
                    * CeilDiv<size_t>(bitsPerDocumentFeature, CHAR_BIT);

                auto dataArray = TConstArrayRef<ui8>((ui8*)featureDataBuffer->data(), sizeInBytes);
                self->AddCatFeaturePart(
                    SafeIntegerCast<ui32>(flatFeatureIdx),
                    0,
                    SafeIntegerCast<ui8>(bitsPerDocumentFeature),
                    NCB::TMaybeOwningConstArrayHolder<ui8>::CreateOwning(
                        dataArray,
                        MakeIntrusive<NCB::TVectorHolder<i64>>(std::move(*featureDataBuffer))
                    )
                );
            }


            // TRawTargetData

            void AddTarget(TConstArrayRef<float> targetData) throw (yexception) {
                self->AddTargetPart(0, AsUnalignedBuf<float>(targetData));
            }

            void AddTarget(TVector<TString>* targetData) throw (yexception) {
                self->AddTargetPart(
                    0,
                    NCB::TMaybeOwningConstArrayHolder<TString>::CreateOwning(std::move(*targetData))
                );
            }

            void AddBaseline(i32 baselineIdx, TConstArrayRef<float> baselineData) throw (yexception) {
                self->AddBaselinePart(
                    0,
                    SafeIntegerCast<i32>(baselineIdx),
                    AsUnalignedBuf<float>(baselineData)
                );
            }

            void AddWeight(TConstArrayRef<float> weightData) throw (yexception) {
                self->AddWeightPart(0, AsUnalignedBuf<float>(weightData));
            }

            void AddGroupWeight(TConstArrayRef<float> groupWeightData) throw (yexception) {
                self->AddGroupWeightPart(0, AsUnalignedBuf<float>(groupWeightData));
            }
        }

        virtual void Finish() = 0;
    };

    struct TDataProviderBuilderOptions {
        //bool GpuDistributedFormat = false;
        //TPathWithScheme PoolPath = TPathWithScheme();
        ui64 MaxCpuRamUsage = Max<ui64>();
        bool SkipCheck = false; // to increase speed, esp. when applying
        //ESparseArrayIndexingType SparseArrayIndexingType = ESparseArrayIndexingType::Undefined;
    };

}

%include "data_provider_builders.h"
