digraph exampleflow
{
subgraph clusterExample
{
start[shape=none];
a[label=demoerror];
b[label=errorparse];
c[label=restjob];
d[label=coderepair];
end[shape=point];
start->a;
a->b[label=false];
a->end[label=true];
b->c;
c->d;
d->end[label=false];
d->a[label=true];
}
}
