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

const root=__dirname;
const data=JSON.parse(fs.readFileSync(root+'/reference-observations.json'));
const baseline=data.find(r=>r.id==='r1-t0-rel0.4-fast0').segments.filter(s=>s.kind==='stationary');
const fits=[];
for(const row of data){
 const params=n=>row.parameters.find(p=>p.name===n);const r=Number.parseFloat(params('CompRatio').text)||100,threshold=Number.parseFloat(params('CompThresh').text),release=Number.parseFloat(params('CompRelease').text),fast=params('CompFast').normalized>0.5;
 if(r===1)continue;
 const points=row.segments.filter(s=>s.kind==='stationary').map((s,i)=>({x:s.levelDb,y:s.steadyGainDb-baseline[i].steadyGainDb}));
 const makeup=points[0].y;
 for(const law of ['rational','exponential']){
 const target=(p,x)=>{const v=Math.max(x-p[1],0);return law==='rational'?p[0]*v*v/(v+p[2]):p[0]*(v-p[2]*(-Math.expm1(-v/p[2])));};
 const result=minimize(p=>p[0]<.1||p[0]>1.5||p[2]<1||p[2]>80?1e7:points.reduce((s,q)=>s+(makeup-target(p,q.x)-q.y)**2,0),[1-1/r,threshold-25,15],[.03,1,1],1800);
 fits.push({id:row.id,ratio:r,threshold,release,fast,law,makeup,parameters:result.x,rms:Math.sqrt(result.y/points.length)});
 }
}
fs.writeFileSync(root+'/individual-transfer-fits.json',JSON.stringify(fits,null,2));
console.log(JSON.stringify(fits.filter(r=>r.ratio===4&&r.threshold===-10),null,2));
for(const law of ['rational','exponential'])console.log(law+' total RMS '+Math.sqrt(fits.filter(r=>r.law===law).reduce((s,r)=>s+r.rms*r.rms,0)/fits.filter(r=>r.law===law).length));
