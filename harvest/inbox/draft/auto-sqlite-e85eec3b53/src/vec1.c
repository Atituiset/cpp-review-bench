// AUTO-DRAFT from sqlite/sqlite PR #f2bfe500012a4fd0c9e7483558774f2d3c571729
#define VEC1_PQ_KMEANS_NITER          30
#define VEC1_PQ_KMEANS_STOPVAL         0.999
/* …（同文件无关代码省略）… */
    ** training set using the current codebook is greater than
    ** VEC1_PQ_KMEANS_STOPVAL (which might be say 0.999) times that of the
    ** previous iteration, end the loop now.  */
    if( iIter>0 && (fTotal / fPrevTotal)>=VEC1_PQ_KMEANS_STOPVAL ){  // <<< BUG ANCHOR
      iIter = VEC1_PQ_KMEANS_NITER;
    }
    fPrevTotal = fTotal;
