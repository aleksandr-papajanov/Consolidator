const fs=require('fs'),path=require('path');
const root=path.resolve(__dirname,'../../..'),sampleRate=44100,segments=[];
for(const [low,high]of[[-20,0],[-32,-12],[-8,0],[-40,-20]]){segments.push({kind:'timing-low',frequency:998.6,levelDb:low,seconds:5});segments.push({kind:'timing-high',frequency:998.6,levelDb:high,seconds:4});segments.push({kind:'timing-release',frequency:998.6,levelDb:low,seconds:5});}
for(const db of[-48,-36,-24,-18,-12,-6,0])segments.push({kind:'stationary-level',frequency:998.6,levelDb:db,seconds:3});
for(const hz of[8,16,32,64,125,250,500,1000,2000,4000,8000,16000,20000])segments.push({kind:'quiet-colour',frequency:hz,levelDb:-48,seconds:2});
let frames=0;for(const s of segments){s.startFrame=frames;frames+=Math.round(s.seconds*sampleRate);}
const b=Buffer.alloc(44+frames*8);b.write('RIFF');b.writeUInt32LE(b.length-8,4);b.write('WAVEfmt ',8);b.writeUInt32LE(16,16);b.writeUInt16LE(3,20);b.writeUInt16LE(2,22);b.writeUInt32LE(sampleRate,24);b.writeUInt32LE(sampleRate*8,28);b.writeUInt16LE(8,32);b.writeUInt16LE(32,34);b.write('data',36);b.writeUInt32LE(frames*8,40);
let phase=0;for(const s of segments){const n=Math.round(s.seconds*sampleRate),amp=10**(s.levelDb/20);for(let i=0;i<n;i++){const value=amp*Math.sin(phase);b.writeFloatLE(value,44+(s.startFrame+i)*8);b.writeFloatLE(value,48+(s.startFrame+i)*8);phase=(phase+2*Math.PI*s.frequency/sampleRate)%(2*Math.PI);}}
const raw=root+'/EmulationLab/RawAudio/Probes/cla2a-level-timing-colour.wav';if(!fs.existsSync(raw))fs.writeFileSync(raw,b);fs.writeFileSync(__dirname+'/segments.json',JSON.stringify({sampleRate,segments},null,2));
const ref='C:/Program Files/Common Files/VST3/WaveShell1-VST3 15.0_x64.vst3',plugin=root+'/ModelPlugins.Vst3/build/ConsolidatorCla2aModel_artefacts/Release/VST3/Consolidator CLA-2A Model.vst3';
const conditions=[];function add(id,g,r){const parameters={Gain:g,'Peak Reduction':r};conditions.push({id,reference:{parameters},candidate:{parameters:{...parameters,Bypass:0}}});}
for(const g of[0,.2,.4,.5,.6,.8,1])add('gain-'+g,g,0);
for(const r of[.1,.2,.3,.4,.5,.6,.7,.8,.9,1])add('reduction-'+r,.5,r);
add('gain-0.7-reduction-0.5',.7,.5);add('gain-0.3-reduction-0.8',.3,.8);
const manifest={name:'cla2a-first-identification',reference:{plugin:ref,query:'CLA-2A Stereo'},candidate:{plugin,model:'vst3-parameters',parameters:[]},sampleRates:[44100],blockSize:128,seed:5489,conditions,probes:['harmonic-grid'],probeSettings:{'harmonic-grid':{minimumFrequency:998.6,maximumFrequency:998.6,pointsPerOctave:1,minimumLevelDb:-48,maximumLevelDb:0,levelStepDb:12,minimumPreRollSeconds:5,minimumMeasurementSeconds:.2,maximumHarmonic:8}},splits:{training:'training.json',validation:'validation.json',listening:'listening',validationFraction:.2}};fs.writeFileSync(__dirname+'/manifest.json',JSON.stringify(manifest,null,2));console.log('Raw input: '+frames/sampleRate+' seconds; '+conditions.length+' conditions.');
