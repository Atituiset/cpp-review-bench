// AUTO-DRAFT from sqlite/sqlite PR #c8922e6f6b90eda21bf81073496efbccd44a2fe9
  }
  // <<< BUG ANCHOR
  for(; ii<nTrain; ii++){
    aBest[ii] = vec1PqBestMatch(
        aCentroid, nCentroid, &aTrain[ii*nElem], nElem, 0
    );
  }
