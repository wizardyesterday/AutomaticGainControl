//******************************************************************
// This program generates the required filter coefficients so that
// an input signal, that is sampled at 256000 S/s can be bandwidth
// limited before wideband FM demodulation is carried out.
// The filter coefficients are computed by the minimax function,
// and the result is an equiripple linear phase FIR filter.
// The filter specifications are listed below.
//
// Pass Band: 0 <= F <= 60000 Hz.
// Transition Band: 60000 < F <= 90000 Hz.
// Stop Band: 90000 < F < 128000 Hz.
// Passband Ripple: 0.1
// Stopband Ripple: 0.001
//
// Note that the filter length will be automatically  calculated
// from the filter parameters.
// Chris G. 07/22/2017
//******************************************************************

// Include the common code.
exec('../Common/utils.sci',-1);

//******************************************************************
// Set up parameters.
//******************************************************************
// Sample rate is 256000 S/s.
Fsample = 256000;

// Passband edge.
Fp = 60000;

// Stopband edge.
Fs = 90000;

// The desired demodulator bandwidth.
F = [0 Fp; Fs Fsample/2];

// Bandwidth of transition band.
deltaF = (Fs - Fp) / Fsample;

// Passband ripple
deltaP = 0.1;

// Stopband ripple.
deltaS = 0.001;

// Number of taps for our filter.
n = computeFilterOrder(deltaP,deltaS,deltaF,Fs)

//******************************************************************
// Generate the FIR filter coefficients and magnitude of frequency
// response..  
//******************************************************************

//------------------------------------------------------------------
// This will be a lowpass filter the preceeds the wideband FM
// demodulator.
//------------------------------------------------------------------
h = eqfir(n,F/Fsample,[1 0],[1/deltaP 1/deltaS]);

// Compute magnitude and frequency points.
[hm,fr] = frmag(h,1024);

//******************************************************************
// Plot magnitudes.
//******************************************************************
set("figure_style","new");

title("Predumodulation Filter - Sample Rate: 256000 S/s");
a = gca();
a.margins = [0.225 0.1 0.125 0.2];
a.grid = [1 1];
a.x_label.text = "F, Hz";
a.y_label.text = "|H|";
plot2d(fr*Fsample,20*log10(hm));

