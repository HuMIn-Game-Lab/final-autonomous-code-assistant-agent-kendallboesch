digraph functionCall 
{ 
    subgraph cluster1 
    { 
        start[shape=none];

        comp[label=syntaxerror]; 
        ep[label=errorparse]; 
        rst[label=resterror]; 
        rp[label=coderepair];
        

        end[shape=point];
        start->comp;
        comp->ep[label=false];
        ep->rst;
        rst->rp;
        rp->comp[label=true];
        rp->end[label=false];
        comp->end[label=true];    
    }   
}
