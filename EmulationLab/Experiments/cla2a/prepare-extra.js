const fs = require('fs');
const manifest = JSON.parse(fs.readFileSync(__dirname + '/manifest.json'));
manifest.name = 'cla2a-controls-between-primary-points';
manifest.conditions = [[.01,0],[.05,0],[.1,0],[.3,0],[.7,0],[.9,0],[.5,.55],[.5,.65],[.5,.75],[.5,.85],[.5,.95],[.535,.598]].map(([g,p]) => ({id:`g${g}-p${p}`,reference:{parameters:{'Gain':g,'Peak Reduction':p}},candidate:{parameters:{'Gain':g,'Peak Reduction':p,'Bypass':0}}}));
fs.writeFileSync(__dirname+'/extra-manifest.json',JSON.stringify(manifest,null,2));
