digraph restFix 
{ 
    subgraph cluster1 
    { 
  
        C[label=resterror]; 
        D[label=coderepair];

        X[shape=point];

        C->D;
        D->X;
    }   
}