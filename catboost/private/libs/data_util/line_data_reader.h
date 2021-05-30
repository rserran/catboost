#pragma once

#include "path_with_scheme.h"

#include <catboost/libs/helpers/exception.h>

#include <library/cpp/object_factory/object_factory.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>

#include <util/stream/file.h>
#include <util/string/escape.h>


namespace NCB {

    struct TDsvFormatOptions {
    public:
        bool HasHeader;
        char Delimiter;
        char NumVectorDelimiter;
        bool IgnoreCsvQuoting;

    public:
        explicit TDsvFormatOptions(
            bool hasHeader = false,
            char delimiter = '\t',
            char numVectorDelimiter = ';',
            bool ignoreCsvQuoting = false
        )
            : HasHeader(hasHeader)
            , Delimiter(delimiter)
            , NumVectorDelimiter(numVectorDelimiter)
            , IgnoreCsvQuoting(ignoreCsvQuoting)
        {
            CB_ENSURE(
                delimiter != numVectorDelimiter,
                "Field delimiter (" << EscapeC(Delimiter) << ") is the same as num vector delimiter ("
                << EscapeC(NumVectorDelimiter) << ')'
            );
        }

        SAVELOAD(HasHeader, Delimiter, NumVectorDelimiter, IgnoreCsvQuoting);
        Y_SAVELOAD_DEFINE(HasHeader, Delimiter, NumVectorDelimiter, IgnoreCsvQuoting);
    };

    struct TLineDataReaderArgs {
        TPathWithScheme PathWithScheme;
        TDsvFormatOptions Format;
        bool KeepLineOrder = true;
    };


    struct ILineDataReader {
        /* if estimate == false, returns number of data lines (w/o header, if present)
           in some cases could be expensive, e.g. for files
           if estimate == true, returns some quick estimation, lower or upper
        */
        virtual ui64 GetDataLineCount(bool estimate = false) = 0;

        /* call before any calls to ReadLine if you need it
           it is an error to call GetHeader after any ReadLine calls
        */
        virtual TMaybe<TString> GetHeader() = 0;

        /* returns true, if data were read
           implementation may return lines in any order
           logical line index is stored to *lineIdx, if lineIdx != nullptr
           not thread-safe
        */
        virtual bool ReadLine(TString* line, ui64* lineIdx = nullptr) = 0;

        virtual bool ReadLine(TString*, TString*, ui64* lineIdx = nullptr) = 0;

        virtual ~ILineDataReader() = default;
    };

    using TLineDataReaderFactory =
        NObjectFactory::TParametrizedObjectFactory<ILineDataReader, TString, TLineDataReaderArgs>;

    THolder<ILineDataReader> GetLineDataReader(const TPathWithScheme& pathWithScheme,
                                               const TDsvFormatOptions& format = TDsvFormatOptions(),
                                               bool keepLineOrder = true);


    int CountLines(const TString& poolFile);

    class TFileLineDataReader : public ILineDataReader {
    public:
        TFileLineDataReader(const TLineDataReaderArgs& args)
            : Args(args)
            , IFStream(args.PathWithScheme.Path)
            , HeaderProcessed(!Args.Format.HasHeader)
        {}

        ui64 GetDataLineCount(bool estimate) override {
            if (estimate) {
                // TODO(espetrov): estimate based one first N lines
                return 1;
            }
            ui64 nLines = (ui64)CountLines(Args.PathWithScheme.Path);
            if (Args.Format.HasHeader) {
                --nLines;
            }
            return nLines;
        }

        TMaybe<TString> GetHeader() override {
            if (Args.Format.HasHeader) {
                CB_ENSURE(!HeaderProcessed, "TFileLineDataReader: multiple calls to GetHeader");
                TString header;
                CB_ENSURE(IFStream.ReadLine(header), "TFileLineDataReader: no header in file");
                HeaderProcessed = true;
                return header;
            }

            return {};
        }

        bool ReadLine(TString* line, ui64* lineIdx) override {
            // skip header if it hasn't been read
            if (!HeaderProcessed) {
                GetHeader();
            }
            if (lineIdx) {
                *lineIdx = LineIndex;
            }
            ++LineIndex;
            return IFStream.ReadLine(*line) != 0;
        }

        bool ReadLine(TString*, TString*, ui64*) override {
            CB_ENSURE(false, "TFileLineDataReader: ReadLine(TString*, TString*, ui64*) is not implemented");
        }

    private:
        TLineDataReaderArgs Args;
        TIFStream IFStream;
        bool HeaderProcessed;
        ui64 LineIndex = 0;
    };

}
