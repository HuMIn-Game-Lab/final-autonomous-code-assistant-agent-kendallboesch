digraph functionCall { 
    subgraph cluster1 
    { 
        Y[shape=none];

        A[label=oneerrorsimple]; 
        B[label=errorparse]; 
        C[label=resterror]; 
        D[label=coderepair];
        

        X[shape=point];
        Y->A;
        A->B[label=false];
        B->C;
        C->D;
        D->A[label=true];
        D->X[label=false];
        A->X[label=true];    
    }   
}