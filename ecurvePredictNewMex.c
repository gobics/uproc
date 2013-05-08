/*
 ============================================================================
 * function: [PredScoresFwd,PredCountsFwd,PredPfamsFwd,PredSeqIs,PredBegs,PredEnds] = ...
        ecurvePredictNewMex(single(ScoresFwd), uint16(PfamsFwd), uint32(SeqPos), ScoreMatFwd, Thresholds);
 * desc.: forward scoring and prediction 
 *
 * author: Peter Meinicke
 * create date: 08/2012
 * last update: 04/2013
 ============================================================================
 */

/* Includes */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "mex.h"
#include "matrix.h"

/* Defines */
#define BUF_SIZE 65536
#define TAB_SIZE 128
#define MASK5 0x001f
#define MASK12 0x0fff
#define MASK14 0x3fff
#define MASK60 0x0fffffffffffffff
#define N_ALPHA 20
#define N_PFAMS 14469 /*Pfam24: 12621, Pfam26: 14469  Kegg: 15368 */
#define N_PREDS_MAX 10000
#define SCORE_THRES -200
#define WLEN 18        
#define SEQ_LEN_CALIB 100.0
#define N_MIN 5.0      

#define MIN(a,b) ((a<b)? (a) : (b))

#define ISALPHA(c) (((c) | 32) - 'a' + 0U <= 'z' - 'a' + 0U)
#define Wi(W,i) ((unsigned int)((W>>(60-i*5)) & 0x1f))

#define MIN_SCORE (-10000)

/* definition of the main-function */
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])	{
    /* nlhs: number of outputs (left hand side)
     * phls: pointer-array to output-fields (mxArrays)
     * nrhs: number of inputs (right hand side)
     * prhs: pointer-array to input-data
     */
    
    /* variable declarations */
    uint16_T indPfam, nPfams, pfam, *Pfams, *PredPfamsAll;
    uint32_T iSeq,  nSeqs, *SeqBegs, *PredSeqIds, *PredBegsAll, *PredEndsAll;
    real32_T *Scores, *ScoreMat, *Thresholds;
    
    int seqBeg, seqEnd, seqLen, overlap, calibFlag;
    unsigned int i, iPred, jPred, iPredOld, iPredNew, iPredAll, iPredAllFst, iPredAllLst, iScore, nPreds, nPredsNew, nValids, nScores, indGC, inLength, nGCs, nLengths;
    unsigned int indScore, indCount, indLength, posiOld, posiNew, pfamMax, count, countMax, iRow, nRows, nThres;
    unsigned int PfamPredInds[N_PFAMS], PredIndsOld[N_PREDS_MAX], PredPosisBeg[N_PREDS_MAX], PredPosisEnd[N_PREDS_MAX], PredPosiOld[N_PREDS_MAX];
    unsigned int PredPfams[N_PREDS_MAX], PredPfamsNew[N_PREDS_MAX], PredCounts[N_PREDS_MAX], PredCountsNew[N_PREDS_MAX];
    real32_T score, scoreMax, scoreSum, scoreThres, PredScores[N_PREDS_MAX], PredScoresNew[N_PREDS_MAX];
    
    real32_T *PredScoresAll, *PredCountsAll;
    real32_T lengthFactor, evalue, evalueThres, evalueScore, *EvalueScoreMat;
    
    /*check number of in- and ouput args*/
    if (nrhs != 4 && nrhs != 5)
        mexErrMsgTxt("Either four or five input arguments expected.");
    if (nlhs != 6)
        mexErrMsgTxt("Six output arguments expected.");
    
    /* read input args */
    nScores = (uint64_T) mxGetM(prhs[0]);
    Scores = (real32_T*) mxGetData(prhs[0]);
    Pfams = (uint16_T*) mxGetData(prhs[1]);
    nSeqs = (uint32_T) mxGetM(prhs[2]) - 1; /* last pointer to begin of empty "dummy" sequence */ 
    SeqBegs = (uint32_T*) mxGetData(prhs[2]);
    ScoreMat = (real32_T*) mxGetData(prhs[3]);
    nRows = (unsigned int) mxGetM(prhs[3]);
        
    if(nrhs > 4){ /* if not in calibration mode ... */
        Thresholds = (real32_T*) mxGetData(prhs[4]);
        calibFlag = 0;
    }
    else{
        scoreThres = -1e4; /*(float) mxGetScalar(prhs[4]);*/
        calibFlag = 1;
    }
    
    /* allocate persistent matlab memory for output */
    plhs[0] = mxCreateNumericMatrix(nScores, 1, mxSINGLE_CLASS, 0);
    PredScoresAll = (real32_T*) mxGetData(plhs[0]);
    plhs[1] = mxCreateNumericMatrix(nScores, 1, mxSINGLE_CLASS, 0);
    PredCountsAll = (real32_T*) mxGetData(plhs[1]);
    plhs[2] = mxCreateNumericMatrix(nScores, 1, mxUINT16_CLASS, 0);
    PredPfamsAll = (uint16_T*) mxGetData(plhs[2]);
    plhs[3] = mxCreateNumericMatrix(nScores, 1, mxUINT32_CLASS, 0);
    PredSeqIds = (uint32_T*) mxGetData(plhs[3]);
  
    plhs[4] = mxCreateNumericMatrix(nScores, 1, mxUINT32_CLASS, 0);
    PredBegsAll = (uint32_T*) mxGetData(plhs[4]);
    plhs[5] = mxCreateNumericMatrix(nScores, 1, mxUINT32_CLASS, 0);
    PredEndsAll = (uint32_T*) mxGetData(plhs[5]);
            
    /* init */
    for(i=0; i<N_PFAMS; PfamPredInds[i++]=0);
    
    /* first loop: */
    iPredAll = 0;
    
    for(iSeq=0; iSeq<nSeqs; iSeq++){
        seqBeg = SeqBegs[iSeq]-1; /* matlab indices (positions) starting with 1 */
        seqEnd = SeqBegs[iSeq+1]-3; /* matlab2c: -1, begin2end: -2, skip-zero-end-letter: -3 */
        seqLen = seqEnd-seqBeg+1;
                
        if(seqLen>=WLEN){
            
            /* first loop: determine max. single scores for first candidate predictions */
            for(nValids=0, nPreds=0, iPred=0, iScore=seqBeg; iScore<=seqEnd; iScore++){
                pfam = Pfams[iScore];
                
                if(pfam>0){
                    nValids++; /* count valid hits for e-value calculation */
                    score = Scores[iScore];
                    indPfam = pfam-1;
                    
                    if(PfamPredInds[indPfam]==0){
                        iPred = nPreds++;
                        PfamPredInds[indPfam] = iPred+1; /* zero (index) is reserved!! */
                        PredPfams[iPred] = pfam;
                        PredCounts[iPred] = 0;
                        PredScores[iPred] = score;
                        PredPosisBeg[iPred] = iScore;
                    }
                    else{
                        iPred = PfamPredInds[indPfam]-1; /* PfamPredInds starting with 1 - zero is reserved!! */
                    }
                    PredCounts[iPred] += 1;
                    PredPosisEnd[iPred] = iScore+WLEN-1;
                    
                    if(score>PredScores[iPred]) /* determine MAX score! */
                        PredScores[iPred] = score;
                    
                }
            }
            
            /* second loop: prepare new candidate predictions for final scoring */
            for(iPred=0, iPredNew=0; iPred<nPreds; iPred++){
                pfam = PredPfams[iPred];
                indPfam = pfam-1;
                if(PredScores[iPred] <= 0){ /* zero threshold for matching scores! */
                    PfamPredInds[indPfam] = 0;
                }
                else{
                    PfamPredInds[indPfam] = iPredNew+1; /* zero (index) is reserved!! */
                    PredScoresNew[iPredNew] = 0;
                    PredCountsNew[iPredNew] = 0;
                    PredPfamsNew[iPredNew] = pfam;
                    PredIndsOld[iPredNew] = iPred;
                    PredPosiOld[iPredNew] = 0;
                    iPredNew++;
                }
            }
            nPredsNew = iPredNew;
            
            /* third loop: compute max-score sums */
            for(iScore=seqBeg; iScore<=seqEnd; iScore++){
                pfam = Pfams[iScore];
                if(pfam > 0){
                    indPfam = pfam-1;
                    iPred = PfamPredInds[indPfam];
                    if(iPred > 0){
                        iPred -= 1;
                        posiNew = iScore;
                        offsetNew = 0; /* zero if forward */
                        if(PredCountsNew[iPred]>0){
                            posiOld = PredPosiOld[iPred];
                            offsetOld = 0; /*PredOffOld[iPred];*/ /* zero if forward, offset=nRows*nScores if reverse */ 
                            overlap = nRows - (posiNew-posiOld); /* nRows = N_POS (#scoring word length, e.g. 18-2=16), not #seq.positions! */
                            if(overlap<0) overlap=0;
                            for(iRow=0; iRow<nRows-overlap; iRow++)
                                PredScoresNew[iPred] += ScoreMat[posiOld*nRows + iRow];
                            for(iRow=0; iRow<overlap; iRow++)
                                if(ScoreMat[offsetNew + posiNew*nRows + iRow] < ScoreMat[offsetOld + posiOld*nRows + nRows-overlap+iRow])
                                    ScoreMat[offsetNew + posiNew*nRows + iRow] =  ScoreMat[offsetOld + posiOld*nRows + nRows-overlap+iRow];
                        }
                        PredPosiOld[iPred] = posiNew;
                        PredCountsNew[iPred] += 1;
                    }
                }
            }
            
            /* fourth loop: final predictions */
            iPredAllFst = iPredAll;
            for(iPred=0; iPred<nPredsNew; iPred++){
                pfam = PredPfamsNew[iPred];
                posiOld = PredPosiOld[iPred];
                scoreSum = PredScoresNew[iPred];
                for(iRow=0; iRow<nRows; iRow++)
                    scoreSum += ScoreMat[posiOld*nRows + iRow];
                
                if(!calibFlag){
                    scoreThres = Thresholds[iSeq]; /* score-threshold */
                }
                
                if(scoreSum > scoreThres){
                    count = PredCountsNew[iPred];
                    iPredOld = PredIndsOld[iPred];
                    PredBegsAll[iPredAll] = PredPosisBeg[iPredOld] - seqBeg + 1; /* matlab index (starting with 1)! */
                    PredEndsAll[iPredAll] = PredPosisEnd[iPredOld] - seqBeg + 1; /* matlab index (starting with 1)! */
                    
                    PredScoresAll[iPredAll] = scoreSum;
                    PredCountsAll[iPredAll] = count;
                    PredPfamsAll[iPredAll] = pfam;
                    PredSeqIds[iPredAll++] = iSeq+1;
                }
                indPfam = pfam-1;
                PfamPredInds[indPfam] = 0; /* clean arrays (only for re-using ...) */
            }
            iPredAllLst = (iPredAll>iPredAllFst)? iPredAll-1 : iPredAllFst;
            
            /* fifth loop: remove overlapping lower-score predictions */
            if(iPredAllLst>iPredAllFst){
                for(iPred=iPredAllFst; iPred<=iPredAllLst; iPred++){
                    for(jPred=iPred+1; jPred<=iPredAllLst; jPred++){
                        if(PredPfamsAll[iPred]>0 && PredPfamsAll[jPred]>0){
                            overlap = PredEndsAll[iPred]-PredBegsAll[jPred]+1;
                            if(overlap>0){
                                if(PredScoresAll[iPred]>PredScoresAll[jPred] && PredEndsAll[jPred]-PredBegsAll[jPred]<2*overlap)
                                    PredPfamsAll[jPred] = 0;
                                else
                                    if(PredScoresAll[iPred]<PredScoresAll[jPred] && PredEndsAll[iPred]-PredBegsAll[iPred]<2*overlap)
                                        PredPfamsAll[iPred] = 0;
                            }
                        }
                    }
                }
            }
            
            
        }
    }
}

