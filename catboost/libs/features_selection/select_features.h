#pragma once

#include "selection_results.h"

#include <catboost/libs/data/data_provider.h>
#include <catboost/private/libs/options/catboost_options.h>
#include <catboost/private/libs/options/features_select_options.h>
#include <catboost/private/libs/options/load_options.h>
#include <catboost/private/libs/options/output_file_options.h>


namespace NCB {
    TFeaturesSelectionSummary SelectFeatures(
        NCatboostOptions::TCatBoostOptions catBoostOptions,
        NCatboostOptions::TOutputFilesOptions outputFileOptions,
        const NCatboostOptions::TPoolLoadParams& poolLoadParams,
        const NCatboostOptions::TFeaturesSelectOptions& featuresSelectOptions,
        const TDataProviders& pools,
        NPar::ILocalExecutor* executor
    );
}
