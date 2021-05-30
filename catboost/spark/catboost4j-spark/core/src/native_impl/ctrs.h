#pragma once

#include <catboost/libs/data/ctrs.h>
#include <catboost/libs/data/objects.h>
#include <catboost/libs/model/ctr_data.h>
#include <catboost/libs/model/online_ctr.h>
#include <catboost/libs/model/model.h>

#include <catboost/private/libs/algo/ctr_helper.h>
#include <catboost/private/libs/algo/online_ctr.h>

#include <util/generic/array_ref.h>
#include <util/generic/fwd.h>
#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/system/tempfile.h>
#include <util/system/types.h>


namespace NPar {
    class TLocalExecutor;
}

namespace  NCatboostOptions {
    class TCatBoostOptions;
}

namespace NCB {
    class TFeaturesLayout;
    class TEstimatedForCPUObjectsDataProviders;
    struct TPrecomputedOnlineCtrMetaData;
}


struct TTargetStatsForCtrs {
    TVector<TVector<int>> LearnTargetClass; // [targetBorderClassifierIdx][objectIdx]
    TVector<int> TargetClassesCount; // [targetBorderClassifierIdx]
};

TCtrHelper GetCtrHelper(
    const NCatboostOptions::TCatBoostOptions& catBoostOptions,
    const NCB::TFeaturesLayout& layout,
    TConstArrayRef<float> learnTarget // can be empty if there's no target data
) throw (yexception);

TTargetStatsForCtrs ComputeTargetStatsForCtrs(
    const TCtrHelper& ctrHelper,
    TConstArrayRef<float> learnTarget,  // can be empty if there's no target data
    NPar::TLocalExecutor* localExecutor
) throw (yexception);


// only Learn and Test in the result are set
void ComputeEstimatedCtrFeatures(
    const TCtrHelper& ctrHelper,
    const NCatboostOptions::TCatBoostOptions& catBoostOptions, // actually only catFeatureParams is used
    const TTargetStatsForCtrs& targetStats,
    const NCB::TQuantizedObjectsDataProviderPtr& learnData,
    const TVector<NCB::TQuantizedObjectsDataProviderPtr>& testData,
    NPar::TLocalExecutor* localExecutor,
    NCB::TEstimatedForCPUObjectsDataProviders* outputData,
    NCB::TPrecomputedOnlineCtrMetaData* outputMeta
) throw (yexception);


class TFinalCtrsCalcer {
public:
    TFinalCtrsCalcer(
        TFullModel* modelWithoutCtrData, // moved into
        const NCatboostOptions::TCatBoostOptions* catBoostOptions,
        const NCB::TQuantizedFeaturesInfo& quantizedFeaturesInfo,
        TConstArrayRef<float> learnTarget,
        TTargetStatsForCtrs* targetStatsForCtrs, // moved into
        const TCtrHelper& ctrHelper,
        NPar::TLocalExecutor* localExecutor
    ) throw(yexception);

    TVector<i32> GetCatFeatureFlatIndicesUsedForCtrs() const throw(yexception);

    void ProcessForFeature(
        i32 catFeatureFlatIdx,
        const NCB::TQuantizedObjectsDataProviderPtr& learnData,
        const TVector<NCB::TQuantizedObjectsDataProviderPtr>& testData
    ) throw(yexception);

    TFullModel GetModelWithCtrData() throw(yexception);

private:
    TFullModel Model;
    NCB::TFeaturesLayoutPtr FeaturesLayout;
    TTargetStatsForCtrs TargetStatsForCtrs;
    NPar::TLocalExecutor* LocalExecutor;
    TTempFile CtrDataFile;
    THolder<TOFStream> CtrDataFileStream;
    THolder<TCtrDataStreamWriter> StreamWriter;
    THashMap<i32, TVector<TModelCtrBase>> CatFeatureFlatIndexToModelCtrsBases;
    TDatasetDataForFinalCtrs DatasetDataForFinalCtrs;
    NCB::TFeaturesArraySubsetIndexing LearnFeaturesSubsetIndexing;
    NCB::TPerfectHashedToHashedCatValuesMap PerfectHashedToHashedCatValuesMap;

    const NCatboostOptions::TCatBoostOptions* CatBoostOptions;
    ui64 CpuRamLimit;
};


