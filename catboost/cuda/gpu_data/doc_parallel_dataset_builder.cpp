#include "doc_parallel_dataset_builder.h"
#include "compressed_index_builder.h"
#include "feature_layout_doc_parallel.h"
#include "dataset_helpers.h"
#include "estimated_features_calcer.h"

NCatboostCuda::TDocParallelDataSetsHolder NCatboostCuda::TDocParallelDataSetBuilder::BuildDataSet(const ui32 permutationCount,
                                                                                                  NPar::TLocalExecutor* localExecutor) {
    TDocParallelDataSetsHolder dataSetsHolder(DataProvider,
                                              FeaturesManager,
                                              LinkedTest);

    TSharedCompressedIndexBuilder<TDataSetLayout> compressedIndexBuilder(*dataSetsHolder.CompressedIndex,
                                                                         localExecutor);

    dataSetsHolder.PermutationDataSets.resize(permutationCount);

    //
    TDataPermutation learnLoadBalancingPermutation = dataSetsHolder.LearnDocPerDevicesSplit->Permutation;

    TCudaBuffer<float, NCudaLib::TStripeMapping> targets;
    TCudaBuffer<float, NCudaLib::TStripeMapping> weights;

    targets.Reset(dataSetsHolder.LearnDocPerDevicesSplit->Mapping);
    weights.Reset(dataSetsHolder.LearnDocPerDevicesSplit->Mapping);

    targets.Write(learnLoadBalancingPermutation.Gather(*DataProvider.TargetData->GetOneDimensionalTarget()));
    weights.Write(learnLoadBalancingPermutation.Gather(GetWeights(*DataProvider.TargetData)));

    for (ui32 permutationId = 0; permutationId < permutationCount; ++permutationId) {
        dataSetsHolder.PermutationDataSets[permutationId] = THolder<TDocParallelDataSet>(new TDocParallelDataSet(DataProvider,
                                                                                    dataSetsHolder.CompressedIndex,
                                                                                    GetPermutation(DataProvider, permutationId),
                                                                                    learnLoadBalancingPermutation,
                                                                                    dataSetsHolder.LearnDocPerDevicesSplit->SamplesGrouping,
                                                                                    TTarget<NCudaLib::TStripeMapping>(targets.ConstCopyView(),
                                                                                                                      weights.ConstCopyView())));
    }

    if (LinkedTest != nullptr) {
        TCudaBuffer<float, NCudaLib::TStripeMapping> testTargets;
        TCudaBuffer<float, NCudaLib::TStripeMapping> testWeights;

        TDataPermutation testLoadBalancingPermutation = dataSetsHolder.TestDocPerDevicesSplit->Permutation;

        testTargets.Reset(dataSetsHolder.TestDocPerDevicesSplit->Mapping);
        testWeights.Reset(dataSetsHolder.TestDocPerDevicesSplit->Mapping);

        testTargets.Write(testLoadBalancingPermutation.Gather(*LinkedTest->TargetData->GetOneDimensionalTarget()));
        testWeights.Write(testLoadBalancingPermutation.Gather(GetWeights(*LinkedTest->TargetData)));

        dataSetsHolder.TestDataSet = THolder<TDocParallelDataSet>(new TDocParallelDataSet(*LinkedTest,
                                                             dataSetsHolder.CompressedIndex,
                                                             GetIdentityPermutation(*LinkedTest),
                                                             testLoadBalancingPermutation,
                                                             dataSetsHolder.TestDocPerDevicesSplit->SamplesGrouping,
                                                             TTarget<NCudaLib::TStripeMapping>(testTargets.ConstCopyView(),
                                                                                               testWeights.ConstCopyView())));
    }

    auto allFeatures = GetLearnFeatureIds(FeaturesManager);
    TVector<ui32> permutationIndependent;
    TVector<ui32> permutationDependent;
    {
        SplitByPermutationDependence(FeaturesManager,
                                     allFeatures,
                                     permutationCount,
                                     &permutationIndependent,
                                     &permutationDependent);
    }

    auto learnMapping = targets.GetMapping();
    TVector<ui32> learnGatherIndicesVec;
    learnLoadBalancingPermutation.FillOrder(learnGatherIndicesVec);
    auto learnGatherIndices = TDatasetPermutationOrderAndSubsetIndexing::ConstructShared(
        DataProvider.ObjectsData->GetFeaturesArraySubsetIndexing(),
        std::move(learnGatherIndicesVec)
    );


    TBinarizationInfoProvider learnBinarizationInfo(FeaturesManager,
                                                    &DataProvider);

    const ui32 permutationIndependentCompressedDataSetId = compressedIndexBuilder.AddDataSet(
        learnBinarizationInfo,
        {"Learn permutation independent features"},
        learnMapping,
        permutationIndependent,
        learnGatherIndices);

    for (ui32 permutationId = 0; permutationId < permutationCount; ++permutationId) {
        auto& dataSet = *dataSetsHolder.PermutationDataSets[permutationId];

        if (permutationDependent.size()) {
            TDataSetDescription description;
            description.Name = TStringBuilder() << "Learn permutation dependent features #" << permutationId;
            dataSet.PermutationDependentFeatures = compressedIndexBuilder.AddDataSet(learnBinarizationInfo,
                                                                                     description,
                                                                                     learnMapping,
                                                                                     permutationDependent,
                                                                                     learnGatherIndices);
        }
        dataSet.PermutationIndependentFeatures = permutationIndependentCompressedDataSetId;
    }

    ui32 testDataSetId = -1;
    if (LinkedTest) {
        TDataSetDescription description;
        description.Name = "Test dataset";
        TVector<ui32> testIndicesVec;
        dataSetsHolder.TestDocPerDevicesSplit->Permutation.FillOrder(testIndicesVec);


        TBinarizationInfoProvider testBinarizationInfo(FeaturesManager,
                                                       LinkedTest);
        auto testObjectsSubsetIndexing = TDatasetPermutationOrderAndSubsetIndexing::ConstructShared(
            LinkedTest->ObjectsData->GetFeaturesArraySubsetIndexing(),
            std::move(testIndicesVec)
        );
        auto testMapping = dataSetsHolder.TestDataSet->GetSamplesMapping();
        testDataSetId = compressedIndexBuilder.AddDataSet(
            testBinarizationInfo,
            description,
            testMapping,
            allFeatures,
            testObjectsSubsetIndexing
        );

        dataSetsHolder.TestDataSet->PermutationIndependentFeatures = testDataSetId;
    }

    compressedIndexBuilder.PrepareToWrite();

    {
        TFloatAndOneHotFeaturesWriter<TDocParallelLayout> floatFeaturesWriter(FeaturesManager,
                                                                              compressedIndexBuilder,
                                                                              DataProvider,
                                                                              permutationIndependentCompressedDataSetId,
                                                                              /*skipExclusiveBundles=*/ false,
                                                                              localExecutor);
        floatFeaturesWriter.Write(permutationIndependent);
    }

    if (LinkedTest) {
        TFloatAndOneHotFeaturesWriter<TDocParallelLayout> floatFeaturesWriter(FeaturesManager,
                                                                              compressedIndexBuilder,
                                                                              *LinkedTest,
                                                                              testDataSetId,
                                                                              /*skipExclusiveBundles=*/ true,
                                                                              localExecutor);
        floatFeaturesWriter.Write(permutationIndependent);
    }

    if (FeaturesManager.GetCtrsCount() + FeaturesManager.GetEstimatedFeatureCount() > 0) {
        WriteCtrsAndEstimatedFeatures(
            dataSetsHolder,
            permutationIndependentCompressedDataSetId,
            testDataSetId,
            permutationCount,
            permutationIndependent,
            permutationDependent,
            &compressedIndexBuilder,
            localExecutor);
    }
    compressedIndexBuilder.Finish();

    return dataSetsHolder;
}

void NCatboostCuda::TDocParallelDataSetBuilder::WriteCtrsAndEstimatedFeatures(
    const NCatboostCuda::TDocParallelDataSetsHolder& dataSetsHolder,
    ui32 permutationIndependentCompressedDataSetId,
    ui32 testDataSetId,
    ui32 permutationCount,
    const TVector<ui32>& permutationIndependent,
    const TVector<ui32>& permutationDependent,
    NCatboostCuda::TSharedCompressedIndexBuilder<NCatboostCuda::TDocParallelLayout>* compressedIndexBuilder,
    NPar::TLocalExecutor* localExecutor
) {
    auto ctrsTarget = BuildCtrTarget(FeaturesManager,
                                        DataProvider,
                                        LinkedTest);

    TMirrorBuffer<ui32> ctrEstimationOrder;
    ctrEstimationOrder.Reset(NCudaLib::TMirrorMapping(DataProvider.GetObjectCount()));
    MakeSequence(ctrEstimationOrder);

    TMirrorBuffer<ui32> testIndices;
    if (LinkedTest) {
        testIndices.Reset(NCudaLib::TMirrorMapping(LinkedTest->GetObjectCount()));
        MakeSequence(testIndices);
    }
    //CTRs
    const auto writeCtrs = [&] (bool useTest, ui32 trainDatasetId, ui32 testDatasetId, const TVector<ui32>& featureIds) {
        TBatchedBinarizedCtrsCalcer ctrsCalcer(FeaturesManager,
                                                *ctrsTarget,
                                                DataProvider,
                                                ctrEstimationOrder,
                                                useTest ? LinkedTest : nullptr,
                                                useTest ? &testIndices : nullptr,
                                                localExecutor);

        TCtrsWriter<TDocParallelLayout> ctrsWriter(FeaturesManager,
                                                    *compressedIndexBuilder,
                                                    ctrsCalcer,
                                                    trainDatasetId,
                                                    testDatasetId);
        ctrsWriter.Write(featureIds);
    };
    writeCtrs(LinkedTest != nullptr, permutationIndependentCompressedDataSetId, testDataSetId, permutationIndependent);
    //TODO: ideally should be combined with CTRs
    const auto writeEstimatedFeatures = [&] (bool useTest, ui32 trainDatasetId, const auto& permutation, const TVector<ui32>& featureIds) {
        TEstimatorsExecutor estimatorsExecutor(FeaturesManager,
                                                Estimators,
                                                permutation,
                                                localExecutor);

        TMaybe<ui32> testId;
        if (useTest) {
            testId = testDataSetId;
        }
        TEstimatedFeaturesWriter<TDocParallelLayout> writer(FeaturesManager,
                                                            *compressedIndexBuilder,
                                                            estimatorsExecutor,
                                                            trainDatasetId,
                                                            testId);
        writer.Write(featureIds);
    };
    writeEstimatedFeatures(LinkedTest != nullptr, permutationIndependentCompressedDataSetId, GetIdentityPermutation(DataProvider), permutationIndependent);

    if (!permutationDependent.empty()) {
        for (ui32 permutationId = 0; permutationId < permutationCount; ++permutationId) {
            auto& ds = *dataSetsHolder.PermutationDataSets[permutationId];
            const TDataPermutation& ctrsEstimationPermutation = ds.GetCtrsEstimationPermutation();
            ctrsEstimationPermutation.WriteOrder(ctrEstimationOrder);

            const bool useTest = (permutationId == 0 && LinkedTest);

            writeCtrs(useTest, ds.PermutationDependentFeatures, testDataSetId, permutationDependent);
            writeEstimatedFeatures(useTest, ds.PermutationDependentFeatures, ctrsEstimationPermutation, permutationDependent);
            CATBOOST_INFO_LOG << "Ctr computation for " << permutationId << " is finished" << Endl;
        }
    }
}
