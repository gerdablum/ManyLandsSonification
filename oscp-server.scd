(
OSCdef.new(
	\audioControl,
	{
		arg msg, time, addr, port;
		msg[1].postln;
		a.set(\gate, msg[1]);
	},
	'/s_control'
);

OSCdef.new(
	\frequencyControl,
	{
		arg msg, time, addr, port;
		a.set(\freq, msg[1]);
	},
	'/s_freq'
);

OSCdef.new(
	\dimensionalityChange,
	{
		arg msg, time, addr, port;
		var x, y, z, w, sound;
		x = msg[10];
		y = msg[11];
		z = msg[12];
		w = msg[13];
		sound = {
			var freqs, ringtimes, amps, sig, env;
			env = XLine.kr(1, 0.02, 0.5, doneAction: 2);
			freqs = [400, 800, 1153, 200];
			ringtimes = [1, 1, 1, 1];
			amps = [x, y, z, w] * [0.2, 0.2, 0.2, 0.2];
			sig = DynKlank.ar(`[freqs, amps, ringtimes ], Impulse.ar(0));
			Pan2.ar(sig, 0) * env;
		}.play;

	},
	'/s_dimensionality_change'
);

)

(

SynthDef(\continuousSound,{|out= 0 freq = 440 ampl = 0.6 gate=1 pan=0.0|

	var sig, env, freqs;
	env = EnvGen.kr(Env.adsr(0.03, 0.1, 1, 0.3), gate);

	freqs = [freq.cpsmidi, freq.cpsmidi + 4, freq.cpsmidi + 7].midicps;
	sig = SinOsc.ar(freq);
	ampl = ampl * SinOsc.kr({exprand(0.2,12)}!3).range(0,1);
	sig = sig * ampl * env;
	sig = Splay.ar(sig) * 0.5;
	Out.ar(out, sig !2);
}).add;

SynthDef(\shortSound, { |freqs= 440|
	var sig, freq, env;
	env = XLine.kr(1, 0.02, 0.5, doneAction: 2);
	freq = XLine.kr(freqs, freqs/2, 0.5, doneAction:2);
	sig = SinOsc.ar(freq) * env;
	Out.ar(0, sig * 0.5 !2);

}).add;
)


(
f = {
			var sig, freq, env, amp;
			env = XLine.kr(1, 0.02, 0.5, doneAction: 2);
			freq = [220, 440, 660, 880];
			amp = [1, 1, 1, 1];
			sig = SinOsc.ar(freq) * amp;
}.play;
)

(
k = {
	var freqs, ringtimes, amps, sig, env;
	env = XLine.kr(1, 0.02, 0.5, doneAction: 2);
	freqs = [400, 800, 1153, 200];
	ringtimes = [1, 1, 1, 1];
	amps = [1 * 0.2, 1 * 0.2, 1 * 0.2, 1 *0.2];
	sig = DynKlank.ar(`[freqs, amps, ringtimes ], Impulse.ar(0));
	Pan2.ar(sig, 0) * env;
}.play;
)

a = Synth.new(\continuousSound);
a.set(\freq, 200);
a.set(\gate, 0);
s.plotTree();

speedSound.free;

s.meter;