#pragma once
// VDSPBiquadF — drop-in replacement for Cloudseed::Biquad on Apple platforms.
//
// Delegates all coefficient computation to an internal Biquad instance and
// replaces the scalar sample loop in Process() with vDSP_biquad (float).
//
// Sign convention is identical: y[n] = b0*x[n]+b1*x[n-1]+b2*x[n-2]-a1*y[n-1]-a2*y[n-2]
// Coefficients from GetVDSPCoeffs() map directly.

#ifdef __APPLE__

#include <Accelerate/Accelerate.h>
#include "Biquad.h"

namespace Cloudseed {

class VDSPBiquadF {
public:
    Biquad::FilterType Type;
    float Frequency;
    float Output;

private:
    Biquad _inner;
    vDSP_biquad_Setup _setup;
    // vDSP requires 2*(N+1) delay elements for N sections; N=1 → 4.
    float _delay[4];

    void rebuildSetup() {
        if (_setup) {
            vDSP_biquad_DestroySetup(_setup);
            _setup = nullptr;
        }
        float fcoeffs[5];
        _inner.GetVDSPCoeffs(fcoeffs);
        // vDSP_biquad_CreateSetup requires double coefficients even for the
        // single-precision vDSP_biquad processing path.
        double dcoeffs[5] = { fcoeffs[0], fcoeffs[1], fcoeffs[2], fcoeffs[3], fcoeffs[4] };
        _setup = vDSP_biquad_CreateSetup(dcoeffs, 1);
    }

public:
    VDSPBiquadF(Biquad::FilterType type, float fs)
        : Type(type), Frequency(fs / 4.0f), Output(0.0f)
        , _inner(type, fs), _setup(nullptr)
    {
        _delay[0] = _delay[1] = _delay[2] = _delay[3] = 0.0f;
        rebuildSetup();
    }

    ~VDSPBiquadF() {
        if (_setup) vDSP_biquad_DestroySetup(_setup);
    }

    VDSPBiquadF(const VDSPBiquadF&) = delete;
    VDSPBiquadF& operator=(const VDSPBiquadF&) = delete;

    void SetSamplerate(float fs) { _inner.SetSamplerate(fs); rebuildSetup(); }
    void SetGainDb(float value)  { _inner.SetGainDb(value); }
    void SetGain(float value)    { _inner.SetGain(value); }

    void Update() {
        _inner.Type      = Type;
        _inner.Frequency = Frequency;
        _inner.Update();
        rebuildSetup();
    }

    void ClearBuffers() {
        _delay[0] = _delay[1] = _delay[2] = _delay[3] = 0.0f;
        Output = 0.0f;
    }

    void Process(float* input, float* output, int len) {
        vDSP_biquad(_setup, _delay, input, 1, output, 1, (vDSP_Length)len);
    }
};

} // namespace Cloudseed

#endif // __APPLE__
