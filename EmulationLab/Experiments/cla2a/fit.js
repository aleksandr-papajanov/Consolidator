const fs = require('fs');
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
const reference=[...JSON.parse(fs.readFileSync(root+'/reference-features.json')),...JSON.parse(fs.readFileSync(root+'/reference-extra-features.json'))];
const gains=reference.filter(r=>r.reduction===0).map(r=>[r.gain,r.levels[0].gainDb]);
gains.push([.535,reference.find(r=>r.gain===.535).levels[0].gainDb]);
// A logarithmic low end plus a cubic describes the output potentiometer.
const gainFn=(p,g)=>p[0]+p[1]*(Math.tanh((g-p[2])/p[3])-Math.tanh(-p[2]/p[3]))+p[4]*(Math.tanh((g-p[5])/p[6])-Math.tanh(-p[5]/p[6]));
const gainFit=minimize(p=>p[1]<0||p[4]<0||p[3]<.01||p[6]<.01?1e8:gains.reduce((s,[g,y])=>s+(gainFn(p,g)-y)**2*(g===.535?4:1),0),[-51.35,45,.1,.14,25,.8,.2],[1,5,.05,.03,5,.05,.03],4500);
const baseline=reference.find(r=>r.id==='gain-0.5');
const rows=reference.filter(r=>r.reduction>0&&r.gain===.5).flatMap(r=>r.levels.map((l,i)=>({p:r.reduction,input:l.inputDb,target:baseline.levels[i].gainDb-l.gainDb})));
const threshold=(p,x)=>{const u=(x-.6)/.4;return p.slice(2).reduceRight((s,v)=>s*u+v,0)};
const reduction=(p,x,level)=>{const d=level-threshold(p,x),k=p[1];return d<=-k/2?0:d>=k/2?p[0]*d:p[0]*(d+k/2)**2/(2*k)};
const curveFit=minimize(p=>p[0]<.5||p[0]>.95||p[1]<.1||p[1]>20?1e8:rows.reduce((s,r)=>s+(reduction(p,r.p,r.input)-r.target)**2/Math.max(1,r.target),0),[.75,4,-13.5,-30,-3,2,0,0],[.01,1,1,1,1,1,1,1],5000);
const timing=JSON.parse(fs.readFileSync(root+'/reference-timing.json')).filter(r=>r.pr>=.5);
function timingError(p, detailed=false) {
    const [fa,sa,fr,sr,ad,rd,w]=p;
    if(fa<.0001||fa>.1||sa<fa||sa>2||fr<.01||fr>2||sr<fr||sr>20||ad<1||ad>100||rd<1||rd>100||w<0||w>1)return 1e8;
    let loss=0,n=0, details=[];
    for(const r of timing){let f=0,s=0;for(const row of r.rows){
        const target=Math.max(0,row.reduction.at(-1)),duration=row.kind==='timing-high'?4:5;
        const tf=(target>f?fa*Math.exp(-target/ad):fr*Math.exp(-target/rd));
        const ts=(target>s?sa*Math.exp(-target/ad):sr*Math.exp(-target/rd));
        const predicted=r.times.map(t=>(1-w)*(target+(f-target)*Math.exp(-(t+.0015)/tf))+w*(target+(s-target)*Math.exp(-(t+.0015)/ts)));
        row.reduction.forEach((y,i)=>{if(r.times[i]<3){loss+=(predicted[i]-y)**2;n++;}});
        if(detailed&&row.kind!=='timing-low'&&r.pr===.6)details.push({kind:row.kind,input:row.input,target,observed:row.reduction,predicted});
        f=target+(f-target)*Math.exp(-duration/tf);s=target+(s-target)*Math.exp(-duration/ts);
    }}
    return detailed?details:loss/n;
}
const timingFit=minimize(p=>timingError(p),[.012,.45,.16,3,16,8,.16],[.003,.1,.05,1,3,2,.04],5000);
const result={gainFit,curveFit,curveErrors:rows.filter(r=>r.target>1).map(r=>({...r,error:reduction(curveFit.x,r.p,r.input)-r.target})),timingFit,timingDetails:timingError(timingFit.x,true)};
fs.writeFileSync(root+'/fit.json',JSON.stringify(result,null,2));
console.log(JSON.stringify(result,null,2));
