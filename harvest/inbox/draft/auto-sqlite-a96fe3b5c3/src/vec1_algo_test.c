// AUTO-DRAFT from sqlite/sqlite PR #31e3b6ff459658c156855c9a9761370b5d02e923
  }
  // <<< BUG ANCHOR
  for(; ii<nTrain; ii++){
    aBest[ii] = vec1PqBestMatch(
        aCentroid, nCentroid, &aTrain[ii*nElem], nElem, 0
    );
  }
