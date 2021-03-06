/*
 * Copyright 2015 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include "SkTypes.h"
#if defined(SK_BUILD_FOR_WIN)

#include "SkXPSDocument.h"
#include "SkStream.h"
#include "SkHRESULT.h"

SkXPSDocument::SkXPSDocument(SkWStream* stream,
                   SkScalar dpi,
                   SkTScopedComPtr<IXpsOMObjectFactory> xpsFactory)
        : SkDocument(stream)
        , fXpsFactory(std::move(xpsFactory))
        , fDevice(SkISize{10000, 10000})
{
    const SkScalar kPointsPerMeter = SkDoubleToScalar(360000.0 / 127.0);
    fUnitsPerMeter.set(kPointsPerMeter, kPointsPerMeter);
    SkScalar pixelsPerMeterScale = SkDoubleToScalar(dpi * 5000.0 / 127.0);
    fPixelsPerMeter.set(pixelsPerMeterScale, pixelsPerMeterScale);
    SkASSERT(fXpsFactory);
    fDevice.beginPortfolio(stream, fXpsFactory.get());
}

SkXPSDocument::~SkXPSDocument() {
    // subclasses must call close() in their destructors
    this->close();
}

SkCanvas* SkXPSDocument::onBeginPage(SkScalar width, SkScalar height) {
    fDevice.beginSheet(fUnitsPerMeter, fPixelsPerMeter, {width, height});
    fCanvas.reset(new SkCanvas(sk_ref_sp(&fDevice)));
    return fCanvas.get();
}

void SkXPSDocument::onEndPage() {
    SkASSERT(fCanvas.get());
    fCanvas->flush();
    fCanvas.reset(nullptr);
    fDevice.endSheet();
}

void SkXPSDocument::onClose(SkWStream*) {
    SkASSERT(!fCanvas.get());
    (void)fDevice.endPortfolio();
}

void SkXPSDocument::onAbort() {}

///////////////////////////////////////////////////////////////////////////////

sk_sp<SkDocument> SkDocument::MakeXPS(SkWStream* stream,
                                      SkScalar dpi) {
    IXpsOMObjectFactory* factory = nullptr;
    auto hr = CoCreateInstance(
        CLSID_XpsOMObjectFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&factory));
    if (!SUCCEEDED(hr)) {
        return nullptr;
    }
    return SkDocument::MakeXPS (stream, factory, dpi);
}

sk_sp<SkDocument> SkDocument::MakeXPS(SkWStream* stream,
                                      IXpsOMObjectFactory* factoryPtr,
                                      SkScalar dpi) {
    SkTScopedComPtr<IXpsOMObjectFactory> factory(SkSafeRefComPtr(factoryPtr));
    return stream && factory
           ? sk_make_sp<SkXPSDocument>(stream, dpi, std::move(factory))
           : nullptr;
}

#endif//defined(SK_BUILD_FOR_WIN)
