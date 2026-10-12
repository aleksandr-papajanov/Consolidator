const fs=require('fs');
function minimize(fn, initial, scales, iterations = 1600) {
    const n = initial.length;
    let simplex = [initial, ...initial.map((_, i) => initial.map((v, j) => v + (i === j ? scales[i] : 0)))].map(x => ({x, y:fn(x)}));
    for(let k=0;k<iterations;k++) {
        simplex.sort((a,b)=>a.y-b.y);
        const c=Array(n).fill(0);for(let i=0;i<n;i++)for(let j=0;j<n;j++)c[j]+=simplex[i].x[j]/n;
        const point=(scale)=>{const x=c.map((v,j)=>v+scale*(v-simplex[n].x[j]));return{x,y:fn(x)}};
        const r=point(1);
        if(r.y<simplex[0].y){const e=point(2);simplex[n]=e.y<r.y?e:r;}
        else if(r.y<simplex[n-1].y)simplex[n]=r;
        else{const q=point(r.y<simplex[n].y?.5:-.5);if(q.y<Math.min(r.y,simplex[n].y))simplex[n]=q;else for(let i=1;i<=n;i++){const x=simplex[i].x.map((v,j)=>(v+simplex[0].x[j])/2);simplex[i]={x,y:fn(x)};}}
    }
    simplex.sort((a,b)=>a.y-b.y);return simplex[0];
}

const root=__dirname;const c=JSON.parse(fs.readFileSync(root+'/transfer-functions.json')),data=JSON.parse(fs.readFileSync(root+'/reference-observations.json'));
const dot=(a,b)=>a.reduce((s,v,i)=>s+v*b[i],0);
function curve(r,t,rel,fast,level){const lim=+(r===100),q=lim?0:1/r,s=1-q,l=Math.log(rel/.4),T=t/10;const slope=s*dot(c.slope,[1,q,q*q,T,+fast,l,lim]),start=dot(c.start,[1,q,T,+fast,l]),bend=dot(c.bend,[1,q,q*q,q*q*q,T,+fast,l,+fast*l,lim]);const x=Math.max(level-start,0);return slope*x*x/(x+bend);}
function makeup(r,t){const lim=+(r===100),q=lim?0:1/r,s=1-q;return dot(c.makeup,[s,s*q,s*q*q,lim])*Math.max(1-t/10,0)+c.makeupCold*Math.max(-t/10-1,0)*(-Math.expm1(-s/.1));}
const rows=data.filter(r=>!r.id.startsWith('r1-')).map(row=>{const p=n=>row.parameters.find(p=>p.name===n);const r=parseFloat(p('CompRatio').text)||100,t=parseFloat(p('CompThresh').text),rel=parseFloat(p('CompRelease').text),fast=p('CompFast').normalized>.5;return{r,t,rel,fast,m:makeup(r,t),segments:row.segments.map(s=>({...s,target:curve(r,t,rel,fast,s.levelDb),seconds:s.kind==='settle'?2:s.kind==='stationary'?3:s.kind==='low'?2:s.kind==='high'?3:s.kind==='release'?8:s.kind==='frequency-low'?1:2}))};});
function advance(state,target,time,attack,depth,release){if(target>=state){const d=target-state,e=Math.exp(-time/attack);return target-d*e/(1+d/depth*(1-e));}return Math.max(target,state*Math.exp(-time/release));}
function loss(p,detail=false){const [na,fa,sw,nfloor,nscale,npow,ffloor,fscale,fpow,frt,frs]=p;if(na<.001||na>.3||fa<.0001||fa>.01||sw<0||sw>1||nfloor<0||nfloor>1||nscale<.01||nscale>3||npow<.5||npow>3||ffloor<0||ffloor>1||fscale<.01||fscale>3||fpow<.5||fpow>3||frt<.0001||frt>1||frs<0||frs>1)return 1e7;let sum=0,n=0,max=0;for(const r of rows){let f=0,s=0;const a=r.fast?fa:na,fr=frt+frs*r.rel,sr=r.fast?ffloor+fscale*r.rel**fpow:nfloor+nscale*r.rel**npow,w=r.rel===.4?1:5;for(const seg of r.segments){if(seg.gainDb){for(let i=0;i<seg.times.length;i++){const time=seg.times[i]+.0015;const ff=advance(f,seg.target,time,a,1e9,fr),ss=advance(s,seg.target,time,a,1e9,sr);const gain=r.m-ff*(1-sw)-ss*sw;const d=gain-seg.gainDb[i];sum+=w*d*d;n+=w;max=Math.max(max,Math.abs(d));}}f=advance(f,seg.target,seg.seconds,a,1e9,fr);s=advance(s,seg.target,seg.seconds,a,1e9,sr);}}return detail?{rms:Math.sqrt(sum/n),max,n}:sum/n;}
const fit=minimize(p=>loss(p),[.045,.002,.8,.055,.35,1.3,.01,.38,1.65,.025,.02],[.005,.0003,.05,.01,.04,.1,.005,.04,.1,.005,.01],6000);const result={parameters:fit.x,...loss(fit.x,true)};fs.writeFileSync(root+'/timing-functions.json',JSON.stringify(result,null,2));console.log(JSON.stringify(result,null,2));
