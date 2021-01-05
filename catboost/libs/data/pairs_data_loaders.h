#pragma once

#include "loader.h"

#include <catboost/private/libs/data_types/groupid.h>
#include <catboost/private/libs/data_util/path_with_scheme.h>

#include <library/cpp/object_factory/object_factory.h>

#include <util/generic/fwd.h>
#include <util/generic/string.h>
#include <util/system/types.h>


namespace NCB {
    class IDatasetVisitor;


    // pass this struct to to IPairsDataLoader implementation classes constructors
    struct TPairsDataLoaderArgs {
        TPathWithScheme Path;
        TDatasetSubset DatasetSubset;
    };

    struct IPairsDataLoader {
        virtual ~IPairsDataLoader() = default;

        /* call NeedGroupIdToIdxMap first, if it returns true then call SetGroupIdToIdxMap,
         * call Do after this setup
         */
        virtual bool NeedGroupIdToIdxMap() const { return false; }

        // groupIdToIdxMap maps from groupId to groupIdx, it must include only groups from DatasetSubset
        virtual void SetGroupIdToIdxMap(const THashMap<TGroupId, ui32>* groupIdToIdxMap);

        virtual void Do(IDatasetVisitor* visitor) = 0;
    };

    using TPairsDataLoaderFactory =
        NObjectFactory::TParametrizedObjectFactory<IPairsDataLoader,
                                                   TString,
                                                   TPairsDataLoaderArgs>;

    class TDsvFlatPairsLoader : public IPairsDataLoader {
    public:
        explicit TDsvFlatPairsLoader(TPairsDataLoaderArgs&& args)
            : Args(std::move(args))
        {}

        void Do(IDatasetVisitor* visitor) override;

    private:
        TPairsDataLoaderArgs Args;
    };
}
