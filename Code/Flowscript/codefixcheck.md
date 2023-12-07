digraph codeFixJobCheck 
{
    subgraph cluster1
    {
        Y[shape=none];

        A[label=testcodefix];
        X[shape=point];

        Y->A;
        A->X;
    }
}