#include "runtime_text_options.h"
#include "json_helper.h"

#include <util/string/builder.h>

namespace NCatboostOptions {
    TTokenizedFeatureDescription::TTokenizedFeatureDescription()
        : FeatureId("feature_id", "TokenizedFeature")
        , TokenizerId("tokenizer_id", "TokenizerName")
        , DictionaryId("dictionary_id", "DictionaryName")
        , TextFeatureId("text_feature_id", -1)
        , FeatureEstimators("feature_estimators", TVector<TFeatureCalcerDescription>{})
    {
    }

    TTokenizedFeatureDescription::TTokenizedFeatureDescription(
        TString tokenizerId,
        TString dictionaryId,
        ui32 textFeatureIdx,
        TConstArrayRef<TFeatureCalcerDescription> featureEstimators
    )
        : TTokenizedFeatureDescription()
    {
        FeatureId.Set(
            TStringBuilder() << "AutoGenerated:TokenizedTextFeature=" << textFeatureIdx
                << ";Dictionary=" << dictionaryId
        );
        TokenizerId.Set(std::move(tokenizerId));
        DictionaryId.Set(std::move(dictionaryId));
        TextFeatureId.Set(textFeatureIdx);
        FeatureEstimators.Set(
            TVector<TFeatureCalcerDescription>(featureEstimators.begin(), featureEstimators.end())
        );
    }

    void TTokenizedFeatureDescription::Save(NJson::TJsonValue* optionsJson) const {
        SaveFields(optionsJson, FeatureId, TokenizerId, DictionaryId, TextFeatureId, FeatureEstimators);
    }

    void TTokenizedFeatureDescription::Load(const NJson::TJsonValue& options) {
        CheckedLoad(options, &FeatureId, &TokenizerId, &DictionaryId, &TextFeatureId, &FeatureEstimators);
        CB_ENSURE(
            TextFeatureId.IsSet(),
            "TokenizedFeatureDescription: text_feature_id is not specified for tokenized feature "
                << FeatureId.Get()
        );
    }

    bool TTokenizedFeatureDescription::operator==(const TTokenizedFeatureDescription& rhs) const {
        return std::tie(FeatureId, TokenizerId, DictionaryId, TextFeatureId, FeatureEstimators)
            == std::tie(rhs.FeatureId, rhs.TokenizerId, rhs.DictionaryId, rhs.TextFeatureId, rhs.FeatureEstimators);
    }

    bool TTokenizedFeatureDescription::operator!=(const TTokenizedFeatureDescription& rhs) const {
        return !(*this == rhs);
    }

    static void CreateTokenizedFeatures(
        ui32 textFeatureIdx,
        const TVector<TTextFeatureProcessing>& textFeatureProcessings,
        TVector<TTokenizedFeatureDescription>* tokenizedFeatures
    ) {
        THashMap<std::pair<TString, TString>, ui32> digitizersPairToFeatureIdx;

        for (const auto& featureProcessing: textFeatureProcessings) {
            for (const TString& tokenizerId: featureProcessing.TokenizersNames.Get()) {
                for (const TString& dictionaryId: featureProcessing.DictionariesNames.Get()) {
                    for (const auto& featureCalcer: featureProcessing.FeatureCalcers.Get()) {
                        const auto& pair = std::make_pair(tokenizerId, dictionaryId);
                        if (!digitizersPairToFeatureIdx.contains(pair)) {
                            digitizersPairToFeatureIdx[pair] = tokenizedFeatures->size();

                            TTokenizedFeatureDescription tokenizedFeature{
                                tokenizerId,
                                dictionaryId,
                                textFeatureIdx,
                                TVector<TFeatureCalcerDescription>{}
                            };
                            tokenizedFeatures->push_back(tokenizedFeature);
                        }

                        auto& tokenizedFeature = tokenizedFeatures->at(digitizersPairToFeatureIdx[pair]);
                        tokenizedFeature.FeatureEstimators->push_back(featureCalcer);
                    }
                }
            }
        }
    }

    TRuntimeTextOptions::TRuntimeTextOptions(
        const TVector<ui32>& textFeatureIndices,
        const TTextProcessingOptions& textOptions
    )
        : TRuntimeTextOptions()
    {
        for (const auto& tokenizer: textOptions.GetTokenizers()) {
            Tokenizers->insert({tokenizer.TokenizerId.Get(), tokenizer});
        }

        for (const auto& dictionary: textOptions.GetDictionaries()) {
            Dictionaries->insert({dictionary.DictionaryId.Get(), dictionary});
        }

        TVector<TTokenizedFeatureDescription> tokenizedFeatures;
        for (ui32 textFeatureIdx: textFeatureIndices) {
            CreateTokenizedFeatures(
                textFeatureIdx,
                textOptions.GetFeatureProcessing(textFeatureIdx),
                &tokenizedFeatures
            );
        }
        TokenizedFeatures.Set(tokenizedFeatures);
    }

    TRuntimeTextOptions::TRuntimeTextOptions()
        : Tokenizers("tokenizers", TMap<TString, TTextColumnTokenizerOptions>{})
        , Dictionaries("dictionaries", TMap<TString, TTextColumnDictionaryOptions>{})
        , TokenizedFeatures("tokenized_features", TVector<TTokenizedFeatureDescription>{})
    {
    }

    TRuntimeTextOptions::TRuntimeTextOptions(
        TConstArrayRef<TTextColumnTokenizerOptions> tokenizers,
        TConstArrayRef<TTextColumnDictionaryOptions> dictionaries,
        TConstArrayRef<TTokenizedFeatureDescription> tokenizedFeatures)
        : TRuntimeTextOptions()
    {
        for (const auto& tokenizer: tokenizers) {
            Tokenizers->insert({tokenizer.TokenizerId.Get(), tokenizer});
        }
        for (const auto& dictionary: dictionaries) {
            Dictionaries->insert({dictionary.DictionaryId.Get(), dictionary});
        }
        TokenizedFeatures.Set(
            TVector<TTokenizedFeatureDescription>{tokenizedFeatures.begin(), tokenizedFeatures.end()}
        );
    }

    const TTextColumnTokenizerOptions& TRuntimeTextOptions::GetTokenizerOptions(const TString& tokenizerId) const {
        CB_ENSURE(Tokenizers->contains(tokenizerId), "No options for tokenizerId " << tokenizerId);
        return Tokenizers->at(tokenizerId);
    }

    const TTextColumnDictionaryOptions& TRuntimeTextOptions::GetDictionaryOptions(const TString& dictionaryId) const {
        CB_ENSURE(
            Dictionaries->contains(dictionaryId),
            "No options for dictionaryId " << dictionaryId
        );
        return Dictionaries->at(dictionaryId);
    }

    const TTokenizedFeatureDescription& TRuntimeTextOptions::GetTokenizedFeatureDescription(
        ui32 tokenizedFeatureIdx
    ) const {
        return TokenizedFeatures->at(tokenizedFeatureIdx);
    }

    void TRuntimeTextOptions::Save(NJson::TJsonValue* optionsJson) const {
        SaveFields(optionsJson, Tokenizers, Dictionaries, TokenizedFeatures);
    }

    void TRuntimeTextOptions::Load(const NJson::TJsonValue& options) {
        CheckedLoad(options, &Tokenizers, &Dictionaries, &TokenizedFeatures);
    }

    bool TRuntimeTextOptions::operator==(const TRuntimeTextOptions& rhs) const {
        return std::tie(Tokenizers, Dictionaries, TokenizedFeatures) ==
            std::tie(rhs.Tokenizers, rhs.Dictionaries, rhs.TokenizedFeatures);
    }

    bool TRuntimeTextOptions::operator!=(const TRuntimeTextOptions& rhs) const {
        return !(rhs == *this);
    }

    ui32 TRuntimeTextOptions::TokenizedFeatureCount() const {
        return TokenizedFeatures->size();
    }

    const TVector<TTokenizedFeatureDescription>& TRuntimeTextOptions::GetTokenizedFeatureDescriptions() const {
        return TokenizedFeatures;
    }
}
