const fs=require('fs');
// Manual identification: adaptive offline integration of two reduction memories.
const text=fs.readFileSync(__dirname+'/fit.js','utf8');
eval(text.slice(0,text.indexOf('const root=__dirname;')).replace('function minimize','global.minimize = function'));
const reference=JSON.parse(fs.readFileSync(__dirname+'/reference-features.json'));
const base=reference.find(r=>r.id==='gain-0.5');
const timing=JSON.parse(fs.readFileSync(__dirname+'/reference-timing.json')).filter(r=>r.pr>=.5);
const levels=reference.filter(r=>r.id.startsWith('reduction-')).map(r=>({pr:r.reduction,values:r.levels.map((l,i)=>[l.inputDb,Math.max(0,base.levels[i].gainDb-l.gainDb)])}));
function steady(pr,input){const a=levels.find(r=>r.pr===pr).values;for(let i=1;i<a.length;i++)if(input<=a[i][0])return a[i-1][1]+(a[i][1]-a[i-1][1])*(input-a[i-1][0])/(a[i][0]-a[i-1][0]);return a.at(-1)[1];}
function objective(p,details=false){
 const [fa,sa,releaseScale,floor,w,targetScale,chargeCap]=p;
 if(fa<.0001||fa>1||sa<fa||sa>5||releaseScale<.1||releaseScale>20||floor<.01||floor>10||w<.01||w>.6||targetScale<.1||targetScale>100||chargeCap<.1||chargeCap>100)return 1e8;
 let loss=0,n=0;const out=[];
 for(const r of timing){let f=0,s=0;for(let ri=0;ri<r.rows.length;ri++){const row=r.rows[ri];
  const steadyRow=row.kind==='timing-release'?r.rows[ri-2]:row;
  const measuredTarget=steadyRow.reduction.at(-1);
  const target=measuredTarget<.25?0:measuredTarget,duration=row.kind==='timing-high'?4:5;let t=0;
  const predicted=[];
  function advance(end){while(t<end-1e-9){const dt=Math.min(end-t,t<.2?.001:.02);const chargeDrive=Math.sqrt((target+.1)/(target+chargeCap));const ftime=target>f?fa/chargeDrive:releaseScale/((floor+f)*(1+target/targetScale));const stime=target>s?sa/chargeDrive:releaseScale/((floor+s)*(1+target/targetScale));f=target+(f-target)*Math.exp(-dt/ftime);s=target+(s-target)*Math.exp(-dt/stime);t+=dt;}}
  r.times.forEach((v,i)=>{advance(v+.0015);const value=(1-w)*f+w*s;predicted.push(value);const weight=1/Math.max(2,target);loss+=(value-row.reduction[i])**2*weight;n++;});
  advance(duration);
  if(details&&row.kind!=='timing-low')out.push({pr:r.pr,input:row.input,kind:row.kind,observed:row.reduction,predicted});
 }}
 return details?out:loss/n;
}
const result=minimize(p=>objective(p),[.0045,.064,2.4,2.46,.217,6.2,10],[.001,.02,.4,.5,.04,3,3],2600);
fs.writeFileSync(__dirname+'/optical-fit.json',JSON.stringify({result,details:objective(result.x,true)},null,2));console.log(result);
