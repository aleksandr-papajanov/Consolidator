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

const root=__dirname;const data=JSON.parse(fs.readFileSync(root+'/reference-observations.json'));
const f={"slope":[1.099051717463522,-0.3721265417181836,0.645962713960782,0.02603468727594622,-0.03842827336144549,-0.007782831104283541,0.030243343571539228],"start":[-28.222229214936267,8.554821933564469,8.4967018683028,-0.2501270354369142,-0.11830397623867149],"bend":[17.922932376013176,24.42462138169784,-139.2030337713597,296.43275700159364,1.3292100716527708,-7.9331007007292635,-2.47071566079458,2.2162828629602203,-1.998556923057029],"makeup":[2.213822432000152,-1.0188928162256707,0.6354036058766459,0.16981381414198884],"makeupCold":1.57};
const flat=[...f.slope,...f.start,...f.bend,...f.makeup,f.makeupCold];
const basis=(r,kind)=>{const q=r.ratio===100?0:1/r.ratio,t=r.threshold/10,f=+r.fast,l=Math.log(r.release/.4),lim=+(r.ratio===100);return kind==='slope'?[1,q,q*q,t,f,l,lim]:kind==='start'?[1,q,t,f,l]:[1,q,q*q,q*q*q,t,f,l,f*l,lim];};
const rows=data.filter(r=>!r.id.startsWith('r1-')).map(row=>{const p=n=>row.parameters.find(p=>p.name===n);return{ratio:parseFloat(p('CompRatio').text)||100,threshold:parseFloat(p('CompThresh').text),release:parseFloat(p('CompRelease').text),fast:p('CompFast').normalized>.5,levels:row.segments.filter(s=>s.kind==='stationary')};});
const dot=(a,b)=>a.reduce((s,v,i)=>s+v*b[i],0);
function predict(p,r,level){const q=r.ratio===100?0:1/r.ratio,s=1-q;const slope=s*dot(p.slice(0,7),basis(r,'slope')),start=dot(p.slice(7,12),basis(r,'start')),bend=dot(p.slice(12,21),basis(r,'bend'));if(bend<1||slope<0)return NaN;const m=dot(p.slice(21,25),[s,s*q,s*q*q,+(r.ratio===100)])*Math.max(1-r.threshold/10,0)+p[25]*Math.max(-r.threshold/10-1,0)*(-Math.expm1(-s/.1));const x=Math.max(level-start,0);return m-slope*x*x/(x+bend);}
const loss=p=>{let sum=0,n=0;for(const r of rows)for(const point of r.levels){const y=predict(p,r,point.levelDb);if(!Number.isFinite(y))return 1e8;sum+=(y-point.steadyGainDb)**2;n++;}return sum/n;};
const fit=minimize(loss,flat,flat.map(x=>Math.max(.01,Math.abs(x)*.01)),9000);
const p=fit.x;const result={slope:p.slice(0,7),start:p.slice(7,12),bend:p.slice(12,21),makeup:p.slice(21,25),makeupCold:p[25],rms:Math.sqrt(fit.y)};let max=0;for(const r of rows)for(const point of r.levels)max=Math.max(max,Math.abs(predict(p,r,point.levelDb)-point.steadyGainDb));result.max=max;fs.writeFileSync(root+'/transfer-functions.json',JSON.stringify(result,null,2));console.log(JSON.stringify(result,null,2));
