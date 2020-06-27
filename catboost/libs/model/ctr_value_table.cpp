#include "ctr_value_table.h"

#include "flatbuffers_serializer_helper.h"
#include <catboost/libs/model/flatbuffers/ctr_data.fbs.h>

#include <util/generic/fwd.h>
#include <util/generic/ptr.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/system/compiler.h>
#include <util/ysaveload.h>


void TCtrValueTable::Save(IOutputStream* s) const {
    using namespace flatbuffers;
    using namespace NCatBoostFbs;
    TModelPartsCachingSerializer serializer;
    if (HoldsAlternative<TSolidTable>(Impl)) {
        auto& solid = Get<TSolidTable>(Impl);
        auto indexHashOffset = serializer.FlatbufBuilder.CreateVector((const ui8*) solid.IndexBuckets.data(),
                                                sizeof(NCatboost::TBucket) * solid.IndexBuckets.size());
        auto ctrBlob = serializer.FlatbufBuilder.CreateVector(solid.CTRBlob);
        auto ctrValueTable = CreateTCtrValueTable(
            serializer.FlatbufBuilder,
            serializer.GetOffset(ModelCtrBase),
            indexHashOffset,
            ctrBlob,
            CounterDenominator,
            TargetClassesCount);
        serializer.FlatbufBuilder.Finish(ctrValueTable);
    } else {
        auto& thin = Get<TThinTable>(Impl);
        auto indexHashOffset = serializer.FlatbufBuilder.CreateVector((const ui8*) thin.IndexBuckets.data(),
                                                sizeof(NCatboost::TBucket) * thin.IndexBuckets.size());
        auto ctrBlob = serializer.FlatbufBuilder.CreateVector(thin.CTRBlob.data(), thin.CTRBlob.size());
        auto ctrValueTable = CreateTCtrValueTable(
            serializer.FlatbufBuilder,
            serializer.GetOffset(ModelCtrBase),
            indexHashOffset,
            ctrBlob,
            CounterDenominator,
            TargetClassesCount);
        serializer.FlatbufBuilder.Finish(ctrValueTable);
    }
    SaveSize(s, serializer.FlatbufBuilder.GetSize());
    s->Write(serializer.FlatbufBuilder.GetBufferPointer(), serializer.FlatbufBuilder.GetSize());
}

void TCtrValueTable::Load(IInputStream* s) {
    const ui32 size = LoadSize(s);
    TArrayHolder<ui8> arrayHolder = TArrayHolder<ui8>(new ui8[size]);
    s->LoadOrFail(arrayHolder.Get(), size);
    LoadSolid(arrayHolder.Get(), size);
}

void TCtrValueTable::LoadSolid(void* buf, size_t length) {
    Y_UNUSED(length); // TODO(kirillovs): add length validation
    using namespace flatbuffers;
    Impl = TSolidTable();
    auto& solid = Get<TSolidTable>(Impl);
    auto ctrValueTable = flatbuffers::GetRoot<NCatBoostFbs::TCtrValueTable>(buf);
    ModelCtrBase.FBDeserialize(ctrValueTable->ModelCtrBase());
    CounterDenominator = ctrValueTable->CounterDenominator();
    TargetClassesCount = ctrValueTable->TargetClassesCount();
    solid.IndexBuckets.assign((NCatboost::TBucket*)ctrValueTable->IndexHashRaw()->data(),
                              (NCatboost::TBucket*)(ctrValueTable->IndexHashRaw()->data() + ctrValueTable->IndexHashRaw()->size()));

    solid.CTRBlob.assign(ctrValueTable->CTRBlob()->data(),
                         ctrValueTable->CTRBlob()->data() + ctrValueTable->CTRBlob()->size());
}
