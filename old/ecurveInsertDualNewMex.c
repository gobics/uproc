/*
 ============================================================================
 * function: [IndicesFwd, ScoresFwd, ScoreMatFwd] = ecurveInsertDualNewMex(EcurveFwd.EcurveWords, EcurveFwd.EcurveHexas,...
        EcurveFwd.HexIndsLo, EcurveFwd.HexIndsUp, Model.Match.ScoringVec, Seq, Model.Alphabet);
 * desc.: insertion,scoring and classification of protein words
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
#include "mex.h"
#include "matrix.h"

/* Defines */
#define BUF_SIZE 65536
#define TAB_SIZE 128
#define MASK5 0x001f
#define MASK12 0x0fff
#define MASK14 0x3fff
#define MASK60 0x0fffffffffffffff
#define MIN_SCORE (-10000)
#define MAX_SCORE 32767

#define AMASK 0x1f
#define WMASK6 0x3fffffff
#define WMASK 0x0fffffffffffffff
#define WLEN 12
#define WLEN_DUO 18

#define WBIT 5
#define N_ALPHA 20
#define N_ALPHA_5 3200000
#define SHIFT_POS(s) (55-s*5) /* (55-s*5) or (60-s*4) */

#define ISALPHA(c) (((c) | 32) - 'a' + 0U <= 'z' - 'a' + 0U)
#define Wi(W, iAA) ((unsigned int)((W>>(60-iAA*5)) & 0x1f))


/* definition of the main-function */
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])	{
    /* nlhs: number of outputs (left hand side)
     * phls: pointer-array to output-fields (mxArrays)
     * nrhs: number of inputs (right hand side)
     * prhs: pointer-array to input-data
     */
    /* input-vector:
     * char[] filename
     */
    /* output-vectors:
     * uint64 wordCounts6[4^6], wordCounts7[4^7], wordCounts8[4^8]
     */
    
    /* variable declarations */
    /* char const Alphabet[20] = "ASTVILMFYHWCPKRQEDNG";*/
    /*<-blos-tsp-eucl, blos-tsp-jdiv: "AGSTPKRQEDNHYWFMLIVC", pfam-tsp: "WFLMIVCAPTSGNDEQKRHY" sammon: "WCFYLIVMTASQKRENDGHP"; original: "ACDEFGHIKLMNPQRSTVWY" */
    
    unsigned int iAA, iPivot, j, loLim, upLim, aaCount, nAminos, nWeights, nScores, nWords, pos, index, row, col, N_POS, N_HEX;
    uint64_T word12, wordSeq, wordPivot, wordA, wordB, *EcurveWords;
    uint32_T wordHex, indHex, *EcurveHexas;
    uint32_T *Indices, *HexIndsLo, *HexIndsUp;
    real32_T score, scoSum, scoMax, scoreMaxLo, scoreMaxUp, *Scores, *ScoreMat, *ScoreBuf, *ScoreCpy, *WeightVec;
    uint8_T *Aminos, *EcurveScores;
    real32_T ScoreBufferUp[WLEN_DUO];
    real32_T ScoreBufferLo[WLEN_DUO];
   
    unsigned char c, aaInd, aaDrop, aaNext, AminoTab[TAB_SIZE];
    char *cPtr, *Alphabet;
    int eqFlag = 0;
    
    /*check number of in- and ouput args*/
    if (nrhs != 7)
        mexErrMsgTxt("Seven input arguments expected.");
    if (nlhs != 3)
        mexErrMsgTxt("Three output arguments expected.");
    
    /* read input args */
    /* Ecurve 12-words and Hexamers */
    nWords = (uint64_T) mxGetM(prhs[0]);
    EcurveWords = (uint64_T*) mxGetData(prhs[0]);
    EcurveHexas = (uint32_T*) mxGetData(prhs[1]);
    /* Ecurve 6-word intervals */
    HexIndsLo = (uint32_T*) mxGetData(prhs[2]);
    HexIndsUp = (uint32_T*) mxGetData(prhs[3]);
    /* Scoring-weights */
    nWeights = (uint32_T) mxGetM(prhs[4]);
    WeightVec = (real32_T*) mxGetData(prhs[4]);
    /* sequence */
    nAminos = (uint32_T) mxGetM(prhs[5]);
    Aminos = (uint8_T*) mxGetData(prhs[5]);
    nScores = nAminos; /* - WLEN_DUO + 1; */
    
    N_POS = (unsigned int)(nWeights / (N_ALPHA*N_ALPHA));
    N_HEX = N_POS-WLEN;
        
    Alphabet = (char*) mxArrayToString(prhs[6]);
    /* fill AminoTab */
    for(iAA=0; iAA<TAB_SIZE; iAA++){
        
        AminoTab[iAA] = 255;
        
        if(ISALPHA(iAA)){
            cPtr = strchr(Alphabet, toupper((char)iAA));
            if(cPtr!=NULL){
                AminoTab[iAA] =  (unsigned char) (cPtr - Alphabet);
            }
        }
    }
    mxFree(Alphabet);
       
    /* allocate persistent matlab memory for output */
    plhs[0] = mxCreateNumericMatrix(nScores, 1, mxUINT32_CLASS, 0);
    Indices = (uint32_T*) mxGetData(plhs[0]);
    plhs[1] = mxCreateNumericMatrix(nScores, 1, mxSINGLE_CLASS, 0);
    Scores = (real32_T*) mxGetData(plhs[1]);
    plhs[2] = mxCreateNumericMatrix(N_POS, nScores, mxSINGLE_CLASS, 0);
    ScoreMat = (real32_T*) mxGetData(plhs[2]);
    
    word12 = 0;
    wordHex = 0;
    aaCount = 0;
    indHex = 0;
      
    for(iAA=0; iAA<nAminos; iAA++) {
        
        c = Aminos[iAA];
        aaInd = AminoTab[c];
        
        /* mexPrintf("aaInd: %d", aaInd); */
        
        if(aaInd < N_ALPHA){ /* valid Amino-Indices: 0..19 */
            
            if(++aaCount > WLEN){
                aaDrop = (wordHex>>25) & MASK5; /* first 5bit (AA) of hex-word */
                aaNext = (word12>>55) & MASK5; /* first 5bit (AA) of 12-word */
                wordHex = (wordHex<<WBIT) + aaNext;
                indHex = (indHex-aaDrop*N_ALPHA_5) * N_ALPHA  +  aaNext;
                /* mexPrintf("indHex: %d, aaDrop: %d, aaNext: %d, wordHex: %x \n", indHex, aaDrop, aaNext, wordHex); */
            }
            word12 = (word12<<WBIT) + aaInd; /* 5 bits per AA letter */
            
            if(aaCount >= WLEN_DUO){ /* protein words require WLEN_DUO (6+12) valid AA letters*/
                
                wordSeq = word12 & WMASK;
                
                /*************** Inserting ... mexPrintf("Inserting .."); **/
                loLim = HexIndsLo[indHex];
                upLim = HexIndsUp[indHex];
                
                /*mexPrintf("loLimFst = %d, upLimFst: %d\n", loLim, upLim);
                  mexPrintf("iWordUp-iWordLo: %d\n", upLim-loLim);*/
                
                while(upLim-loLim > 1){
                    iPivot = (upLim+loLim) >> 1;
                    wordPivot = EcurveWords[iPivot];
                    if(wordSeq < wordPivot){
                        upLim = iPivot;
                    }
                    else{
                        if(wordSeq > wordPivot){
                            loLim = iPivot;
                        }
                        else{
                            loLim = (upLim = iPivot);
                            eqFlag = 1;
                            break;
                        }
                    }
                }/*while*/
                
                /*mexPrintf("loLim = %d, upLim: %d\n", loLim, upLim);*/
                
                /*************** Scoring ... *******************************/
                /* first scores of leading hexamers ... mexPrintf("Scoring .."); */
                /* to be hard-wired for N_HEX <= 3 ? */
                
                /* mexPrintf("N_POS: %d   N_HEX: %d\n", N_POS, N_HEX); */
                
                /* matching with lower word */
                /*
                wordA = wordHex;
                wordB = EcurveHexas[loLim];
                
                for(scoSum=0, scoMax=MIN_SCORE, pos=0; pos<N_HEX; wordA>>=5, wordB>>=5, pos++){
                    row = wordA&MASK5;
                    col = wordB&MASK5;
                    index = pos*N_ALPHA*N_ALPHA + col*N_ALPHA + row;
                    score = WeightVec[index];
                    ScoreBufferLo[pos] = score;
                    scoSum += score;
                }
                scoreMaxLo = scoSum;
                */
                scoreMaxLo = 0;
                
               /* matching with upper word */
                /*
                wordA = wordHex;
                wordB = EcurveHexas[upLim];  
                
                for(scoSum=0, scoMax=MIN_SCORE, pos=0; pos<N_HEX; wordA>>=5, wordB>>=5, pos++){
                    row = wordA&MASK5;
                    col = wordB&MASK5;
                    index = pos*N_ALPHA*N_ALPHA + col*N_ALPHA + row;
                    score = WeightVec[index];
                    ScoreBufferUp[pos] = score;
                    scoSum += score;
                }
                scoreMaxUp = scoSum;
                */
                scoreMaxUp = 0; N_HEX = 0;
                
                /* mexPrintf("scoreMaxLo=%d, scoreMaxUp=%d, indHex: %d\n", scoreMaxLo, scoreMaxUp, indHex); */
                
                /***************/
                /* ... then scores of remaining 12-words */
                /* 'before'-alignment with prior 'loLim' DB fragment(word) */
                wordA = wordSeq;
                wordB = EcurveWords[loLim];
                
                for(scoSum=0, scoMax=0, pos=N_HEX; pos<N_POS; wordA>>=5, wordB>>=5, pos++){
                    row = wordA&MASK5;
                    col = wordB&MASK5;
                    index = pos*N_ALPHA*N_ALPHA + col*N_ALPHA + row;
                    score = WeightVec[index];
                    ScoreBufferLo[pos] = score;
                    scoSum += score;
                }
                scoreMaxLo += scoSum; /* scoMax */
                                                
                /* 'after'-alignment with subsequent 'upLim' DB fragment(word) */
                wordA = wordSeq;
                wordB = EcurveWords[upLim];
                
                for(scoSum=0, scoMax=0, pos=N_HEX; pos<N_POS; wordA>>=5, wordB>>=5, pos++){
                    row = wordA&MASK5;
                    col = wordB&MASK5;
                    index = pos*N_ALPHA*N_ALPHA + col*N_ALPHA + row;
                    score = WeightVec[index];
                    ScoreBufferUp[pos] = score;
                    scoSum += score;
                }
                scoreMaxUp += scoSum;
                                
                /* mexPrintf("Copy buffer ...\n"); */
                
                Scores[iAA+1-WLEN_DUO] = (scoreMaxUp>=scoreMaxLo)? scoreMaxUp : scoreMaxLo;
                Indices[iAA+1-WLEN_DUO] = ((scoreMaxUp>=scoreMaxLo)? upLim : loLim) + 1; /* matlab indexing! */
                
                ScoreBuf = (scoreMaxUp>=scoreMaxLo)? ScoreBufferUp : ScoreBufferLo;
                ScoreCpy = ScoreMat + (iAA+1-WLEN_DUO)*N_POS;
                for(pos=0; pos<N_POS; pos++){
                    ScoreCpy[pos] = ScoreBuf[pos];
                }
                
            }/* if (complete word) */
        }
        else{ /* no valid Amino index! */
            word12 = 0; /* start with a completely new word */
            wordHex = 0; /* start with a completely new word */
            aaCount = 0;
            indHex = 0;
            /* if(iAA+1 + WLEN_DUO > nAminos) break; */
        }
        
    } /* for iAA */
    /*mexPrintf("last iAA: %d\n", iAA);*/
}
    