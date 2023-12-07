digraph functionCall { 
    subgraph cluster1 
    { 
        Y[shape=none];
        A[label=demoerror]; 
        B[label=errorparse]; 
        C[label=resterror]; 
        D[label=coderepair];


        Y->A;
        X[shape=point];

        A->B[label=false];
        B->C;
        C->D;
        D->X;
        
        A->X[label=true];    
    }   
}