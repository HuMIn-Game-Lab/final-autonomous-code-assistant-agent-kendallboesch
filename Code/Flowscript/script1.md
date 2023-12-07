digraph ex
{
subgraph clusterEx
{
a[label=demoerror];
b[label=errorparse];
end[shape=point];
a->b[label=false];
a->end[label=true];
b->end;
}
}
