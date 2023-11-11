/*******************************************************************************
 ** Name: blackbox
 ** Purpose: What the programm should do.
 ** Author: (JE) Jens Elstner <jens.elstner@bka.bund.de>
 *******************************************************************************
 ** Date        User  Log
 **-----------------------------------------------------------------------------
 ** 31.03.2018  JE    Created program.
 ** 01.05.2018  JE    Added '-B' for printing no board between beams.
 ** 01.05.2018  JE    Changed '-B' to '-b' for inverse logic.
 ** 10.08.2018  JE    Adjust some conversion warnings.
 ** 26.01.2020  JE    Now use functions, includes and defines from 'stdfcns.c'.
 ** 27.09.2020  JE    Now use 'stdfcns.c' v0.7.1 and use g_csMename.
 ** 27.09.2020  JE    Added 'b' to print board in loop when wanted.
 ** 27.09.2020  JE    Adjusted usage text apropiately.
 ** 20.04.2021  JE    Now use 'c_dynamic_arrays_macros.h'.
 ** 24.09.2023  JE    Refactored the git from single archive file.
 ** 24.09.2023  JE    Now uses latest libs and deleted unused.
 *******************************************************************************/


//******************************************************************************
//* includes & namespaces

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "c_string.h"
#include "c_dynamic_arrays_macros.h"


//******************************************************************************
//* defines & macros

#define ME_VERSION "0.6.3"
cstr g_csMename;

#define ERR_NOERR 0x00
#define ERR_ARGS  0x01
#define ERR_FILE  0x02
#define ERR_ELSE  0xff

#define sERR_ARGS "Argument error"
#define sERR_FILE "File error"
#define sERR_ELSE "Unknown error"

// Board
#define CELL_EMPTY  0x00
#define CELL_BORDER 0x01
#define CELL_ATOM   0x02

#define BOARD_NEUTRAL  0x00
#define BOARD_SOLUTION 0x01

#define DIR_NONE  0x00
#define DIR_UP    0x01
#define DIR_LEFT  0x02
#define DIR_DOWN  0x03
#define DIR_RIGHT 0x04

#define ATOM_NONE   0x00
#define ATOM_CENTER 0x01
#define ATOM_LEFT   0x02
#define ATOM_RIGHT  0x03

#define SCORE_ATOM      -5
#define SCORE_EXIT      -3
#define SCORE_REFLECTED -2
#define SCORE_ABSORBED  -1


//******************************************************************************
//* outsourced standard functions, includes and defines

#include "stdfcns.c"


//******************************************************************************
//* typedefs

// Arguments and options.
typedef struct s_options {
  int iAtomNo;
  int iWidth;
  int iSize;
  int iCellNo;
  int bPrtBrd;
} t_options;

// Arguments and options.
typedef struct s_score {
  int  iMissedAtoms;
  int  iAbsorbed;
  int  iReflected;
  int  iExited;
} t_score;

// Create dynamic array struct.
s_array(cstr);


//******************************************************************************
//* Global variables

// Arguments
t_options     g_tOpts;    // CLI options and arguments.
t_array(cstr) g_tArgs;    // Free arguments.
t_score       g_tScore;
int*          g_paiGrid;


//******************************************************************************
//* Functions

/*******************************************************************************
 * Name:  usage
 * Purpose: Print help text and exit program.
 *******************************************************************************/
void usage(int iErr, char* pcMsg) {
  cstr csMsg = csNew(pcMsg);

  // Print at least one newline with message.
  if (csMsg.len != 0)
    csCat(&csMsg, csMsg.cStr, "\n");

  csSetf(&csMsg, "%s"
//|************************ 80 chars width ****************************************|
  "usage: %s [-a n] [-s n] [-b]\n"
  "       %s [-h|--help|-v|--version]\n"
  " This program plays a decent game of BlackBox.\n"
  " Per default it contents of a 8 x 8 grid with 4 hidden atoms.\n"
  " \n"
  " Here is an example:\n"
  " \n"
  "      32  31  30  29  28  27  26  25\n"
  "     +---+---+---+---+---+---+---+---+\n"
  "   1 |   |   |   |   |   |   |   |   | 24\n"
  "     +---+---+---+---+---+---+---+---+\n"
  "   2 |   |   | X |   |   |   |   |   | 23\n"
  "     +---+---+---+---+---+---+---+---+\n"
  "   3 |   |   |   |   |   |   |   |   | 22\n"
  "     +---+---+---+---+---+---+---+---+\n"
  "   4 |   |   |   |   |   |   |   |   | 21\n"
  "     +---+---+---+---+---+---+---+---+\n"
  "   5 |   |   |   |   |   |   |   |   | 20\n"
  "     +---+---+---+---+---+---+---+---+\n"
  "   6 |   |   |   | X |   |   |   |   | 19\n"
  "     +---+---+---+---+---+---+---+---+\n"
  "   7 |   |   |   |   |   |   |   |   | 18\n"
  "     +---+---+---+---+---+---+---+---+\n"
  "   8 |   |   | X | X |   |   |   |   | 17\n"
  "     +---+---+---+---+---+---+---+---+\n"
  "       9  10  11  12  13  14  15  16\n"
  " \n"
  " You will be prompted to input one of the number on the edge\n"
  " where the probe beam will start its way through the blackbox.\n"
  " As answer you get either the edge number the beam exits or\n"
  " 'absorbed', if the beam hits an atom head on.\n"
  " \n"
  " The beam movement will be resolved in this order:\n"
  " 1. Is an atom in front of the beam, it will be absorbed.\n"
  " 2. Is an atom in the left front of the beam, it will be\n"
  "    deflected by 90° to the right.\n"
  " 3. Is an atom in the right front of the beam, it will be\n"
  "    deflected by 90° to the left.\n"
  " 4. Is an atom in the right front and left front of the beam,\n"
  "    it will be deflected back by 180°.\n"
  " \n"
  " If you enter 'e' at the prompt the program  will ask you for the\n"
  " coordinates of each atom hidden and then print the score according to your\n"
  " input. If you enter 'q' at the prompt the programm just quit.\n"
  " If you enter 'b' at the prompt the empty board will be redrawn.\n"
  " \n"
  "  -a n:          count of atoms hidden (default 4)\n"
  "  -s n:          size of blackbox grid n x n (default 8)\n"
  "  -b:            print board after each attempt\n"
  "  -h|--help:     print this help\n"
  "  -v|--version:  print version of program\n"
//|************************ 80 chars width ****************************************|
         ,csMsg.cStr,
         g_csMename.cStr, g_csMename.cStr
        );

  if (iErr == ERR_NOERR)
    printf("%s", csMsg.cStr);
  else
    fprintf(stderr, "%s", csMsg.cStr);

  csFree(&csMsg);

  exit(iErr);
}

/*******************************************************************************
 * Name:  dispatchError
 * Purpose: Print out specific error message, if any occurres.
 *******************************************************************************/
void dispatchError(int rv, const char* pcMsg) {
  cstr csMsg = csNew(pcMsg);
  cstr csErr = csNew("");

  if (rv == ERR_NOERR) return;

  if (rv == ERR_ARGS) csCat(&csErr, csErr.cStr, sERR_ARGS);
  if (rv == ERR_FILE) csCat(&csErr, csErr.cStr, sERR_FILE);
  if (rv == ERR_ELSE) csCat(&csErr, csErr.cStr, sERR_ELSE);

  // Set to '<err>: <message>', if a message was given.
  if (csMsg.len != 0) csSetf(&csErr, "%s: %s", csErr.cStr, csMsg.cStr);

  usage(rv, csErr.cStr);
}

/*******************************************************************************
 * Name:  getOptions
 * Purpose: Filters command line.
 *******************************************************************************/
void getOptions(int argc, char* argv[]) {
  cstr csArgv = csNew("");
  int  iArg   = 0;
  int  iChar  = 0;
  char cOpt   = 0;

  // Set defaults.
  g_tOpts.iAtomNo = 4;
  g_tOpts.iSize   = 8;
  g_tOpts.bPrtBrd = 0;

  // Set score to zero.
  g_tScore.iMissedAtoms = 0;
  g_tScore.iAbsorbed    = 0;
  g_tScore.iReflected   = 0;
  g_tScore.iExited      = 0;

  // Init free argument's dynamic array.
  daInit(cstr, g_tArgs);

  // Loop all arguments from command line POSIX style (except program name).
  iArg = 1;
  while (iArg < argc) {
next_argument:
    shift(&csArgv, &iArg, argc, argv);
    if(strcmp(csArgv.cStr, "") == 0)
      continue;

    // Long options:
    if (csArgv.cStr[0] == '-' && csArgv.cStr[1] == '-') {
      if (!strcmp(csArgv.cStr, "--help")) {
        usage(ERR_NOERR, "");
      }
      if (!strcmp(csArgv.cStr, "--version")) {
        version();
      }
      dispatchError(ERR_ARGS, "Invalid long option");
    }

    // Short options:
    if (csArgv.cStr[0] == '-') {
      for (iChar = 1; iChar < csArgv.len; ++iChar) {
        cOpt = csArgv.cStr[iChar];
        if (cOpt == 'h') {
          usage(ERR_NOERR, "");
        }
        if (cOpt == 'v') {
          version();
        }
        if (cOpt == 'a') {
          if (! getArgLong((ll*) &g_tOpts.iAtomNo, &iArg, argc, argv, ARG_CLI, NULL))
            dispatchError(ERR_ARGS, "No valid count of atoms or missing");
          continue;
        }
        if (cOpt == 's') {
          if (! getArgLong((ll*) &g_tOpts.iSize, &iArg, argc, argv, ARG_CLI, NULL))
            dispatchError(ERR_ARGS, "No valid size or missing");
          continue;
        }
        if (cOpt == 'b') {
          g_tOpts.bPrtBrd = 1;
          continue;
        }
        dispatchError(ERR_ARGS, "Invalid short option");
      }
      goto next_argument;
    }
    // Else, it's just a filename.
    daAdd(cstr, g_tArgs, csNew(csArgv.cStr));
  }

  // Sanity check of arguments and flags.
  if (g_tArgs.sCount != 0)
    dispatchError(ERR_ARGS, "No file names needed");

  // Grids cell count is size plus two edges squared.
  g_tOpts.iWidth  = g_tOpts.iSize  + 2;
  g_tOpts.iCellNo = g_tOpts.iWidth * g_tOpts.iWidth;

  // sizeof() yields an unsigned integer!
  g_paiGrid = (int*) malloc(sizeof(int) * (uint) g_tOpts.iCellNo);

  // Free string memory.
  csFree(&csArgv);

  return;
}

/*******************************************************************************
 * Name:  printIntro
 * Purpose: Prints intro text for the game.
 *******************************************************************************/
void printIntro(void) {
  printf("This is blackbox. In this game you try to figure out where atoms are\n");
  printf("placed on a grid of cells. To get informations about the locations of\n");
  printf("the hidden atoms you can fire a beam into the blackbox via the grid's\n");
  printf("edges.\n");
  printf("\n");
  printf("You enter the number of the edge cell you want to explore and as the\n");
  printf("result you get the cell's number where the beam exits the blackbox.\n");
  printf("If the beam exits where it entered, you get the result 'Reflected',\n");
  printf("if the beam gets swallowed by an atom the result will be 'Absorbed'.\n");
  printf("During it's way through the blackbox grid, the beam will be deflected\n");
  printf("by an atom beeing one cell at the right or left of the beams path to\n");
  printf("the opposite direction by 90 degree and absorbed by an atom directly in\n");
  printf("front of the beam's path.\n");
  printf("\n");
  printf("      16  15  14  13\n");
  printf("     +---+---+---+---+\n");
  printf("   1 |   |   |   |   | 12\n");
  printf("     +---+---+---+---+\n");
  printf("   2 |   |   | X |   | 11\n");
  printf("     +---+---+---+---+\n");
  printf("   3 |   |   |   |   | 10\n");
  printf("     +---+---+---+---+\n");
  printf("   4 |   | X | X |   |  9\n");
  printf("     +---+---+---+---+\n");
  printf("       5   6   7   8\n");
  printf("\n");
  printf("In this example a beam from edge cell number 2 or 7 gets absorbed. A\n");
  printf("beam from 10 or 5 gets reflected and a beam from 1 exits at 15, a beam\n");
  printf("from 16 at 3.\n");
  printf("\n");
  printf("If you think you know where all atoms are you can enter (e)nd or\n");
  printf("(f)inish to proceed to enter your solution or enter (q)uit to exit the\n");
  printf("current game.\n");
  printf("\n");
  printf("After you entered the guessed locations of all the hidden atoms, you get\n");
  printf("the result of this game, which is calculated as follows:\n");
  printf("Each wrong guessed atom %d points\n", SCORE_ATOM);
  printf("Each beam that exited   %d points\n", SCORE_EXIT);
  printf("Each reflected beam     %d points\n", SCORE_REFLECTED);
  printf("Each absorbed beam      %d points\n", SCORE_ABSORBED);
}

/*******************************************************************************
 * Name:  cellToXY
 * Purpose: Converts a cell index into x and y coordinates.
 *******************************************************************************/
void cellToXY(int iCell, int* piX, int* piY) {
  *piX = iCell % g_tOpts.iWidth;
  *piY = iCell / g_tOpts.iWidth;
}

/*******************************************************************************
 * Name:  cellFromXY
 * Purpose: Converts x and y coordinates into a cell index.
 *******************************************************************************/
void cellFromXY(int* piCell, int iX, int iY) {
  *piCell = iX + g_tOpts.iWidth * iY;
}

/*******************************************************************************
 * Name:  createBoard
 * Purpose: Creates the board with border and atoms.
 *******************************************************************************/
void createBoard() {
  int iCell = 0;
  int iX    = 0;
  int iY    = 0;

  // Internal 4x4 grid looks like this:
  //     16151413
  //   1 1 1 1 1 1
  // 1 1 0 0 0 0 1 12
  // 2 1 0 2 0 0 1 11
  // 3 1 0 0 0 0 1 10
  // 4 1 0 0 2 0 1  9
  //   1 1 1 1 1 1
  //     5 6 7 8

  // Create default board.
  for (iY = 0; iY < g_tOpts.iWidth; ++iY) {
    for (iX = 0; iX < g_tOpts.iWidth; ++iX) {
       cellFromXY(&iCell, iX, iY);
       if (iX == 0 || iX == g_tOpts.iWidth - 1 ||
           iY == 0 || iY == g_tOpts.iWidth - 1)
         g_paiGrid[iCell] = CELL_BORDER;
       else
         g_paiGrid[iCell] = CELL_EMPTY;
    }
  }

  // Seed pseudo random generator;
  srand((uint) time(NULL));

  // Set atoms into the board.
  for (int i = 0; i < g_tOpts.iAtomNo; ++i) {
    // First set iCell to non empty cell.
    iCell = 0;
    // Get new cell, until it's empty.
    while (g_paiGrid[iCell] != CELL_EMPTY)
      iCell = rand() % g_tOpts.iCellNo;
    // Set atom to this cell.
    g_paiGrid[iCell] = CELL_ATOM;
  }
}

/*******************************************************************************
 * Name:  printBoard
 * Purpose: Prints the board with or without the solution.
 *******************************************************************************/
void printBoard(int bWithSolution) {
  cstr csLine = csNew("");
  int  iCell  = 0;
  int  iX     = 0;
  int  iY     = 0;

  //     16  15  14  13
  //    +---+---+---+---+
  //  1 |   |   |   |   | 12
  //    +---+---+---+---+
  //  2 |   |   | X |   | 11
  //    +---+---+---+---+
  //  3 |   |   |   |   | 10
  //    +---+---+---+---+
  //  4 |   | X | X |   |  9
  //    +---+---+---+---+
  //      5   6   7   8

  // Assemble horizontal line.
  csSet(&csLine, "+");
  for (int i = 0; i < g_tOpts.iSize; ++i)
    csCat(&csLine, csLine.cStr, "---+");

  // Help text.
  printf("\n\n");
  printf("Atoms hidden = %d\n\n", g_tOpts.iAtomNo);

  // Top numbers.
  printf("    ");
  for (iX = 0; iX < g_tOpts.iSize; ++iX)
    printf("%3d ", 4 * g_tOpts.iSize - iX);
  printf("\n");

  // First horizontal line.
  printf("    %s\n", csLine.cStr);

  // Rest of the board.

  // Cells line plus horizontal line 'iSize' times.
  for (iY = 0; iY < g_tOpts.iSize; ++iY) {

    // Left number.
    printf("%3d |", iY + 1);

    for (iX = 0; iX < g_tOpts.iSize; ++iX) {
      cellFromXY(&iCell, iX + 1, iY + 1);
      if (bWithSolution == BOARD_SOLUTION && g_paiGrid[iCell] == CELL_ATOM)
        printf(" X |");
      else
        printf("   |");
    }
    // Right number.
    printf("%3d ", 3 * g_tOpts.iSize - iY);
    // Horizontal line.
    printf("\n    %s\n", csLine.cStr);
  }

  // Bottom numbers.
  printf("    ");
  for (iX = 1; iX <= g_tOpts.iSize; ++iX)
    printf("%3d ", g_tOpts.iSize + iX);
  printf("\n\n");
}

/*******************************************************************************
 * Name:  getEntryNode
 * Purpose: Translate Entry number (iBeam) into according edge cell (iX, iY).
 *******************************************************************************/
cstr getEntryNode(int* piEntryCell, int* piDirection) {
  cstr csBeam = csNew("");
  int  iBeam  = 0;
  int  iX     = 0;
  int  iY     = 0;

  csInput("Enter beam 's entry number: ", &csBeam);
  iBeam = (int) cstr2ll(csBeam);

  // Security check.
  if (iBeam < 1 || iBeam > g_tOpts.iCellNo - 1) {
    *piEntryCell = -1;
    return csBeam;
  }

  // Differentiate at which edge we are.
  if (iBeam >= 1 && iBeam <= g_tOpts.iSize) {
    iX = 0;
    iY = iBeam;
    *piDirection = DIR_RIGHT;
  }
  if (iBeam >= g_tOpts.iSize + 1 && iBeam <= 2 * g_tOpts.iSize) {
    iX = iBeam - g_tOpts.iSize;
    iY = g_tOpts.iWidth - 1;
    *piDirection = DIR_UP;
  }
  if (iBeam >= 2 * g_tOpts.iSize + 1 && iBeam <= 3 * g_tOpts.iSize) {
    iX = g_tOpts.iWidth - 1;
    iY = (3 * g_tOpts.iSize + 1) - iBeam;
    *piDirection = DIR_LEFT;
  }
  if (iBeam >= 3 * g_tOpts.iSize + 1 && iBeam <= 4 * g_tOpts.iSize) {
    iX = (4 * g_tOpts.iSize + 1) - iBeam;
    iY = 0;
    *piDirection = DIR_DOWN;
  }

  cellFromXY(piEntryCell, iX, iY);

  return csBeam;
}

/*******************************************************************************
 * Name:  getExitNode
 * Purpose: Translate exit cell into beam's exit node number.
 *******************************************************************************/
int getExitNode(int iCell) {
  int  iBeam = 0;
  int  iX    = 0;
  int  iY    = 0;

  cellToXY(iCell, &iX, &iY);

  // Determin on wich edge we are.
  if (iX == 0)                  iBeam = iY;
  if (iY == g_tOpts.iWidth - 1) iBeam = iX + g_tOpts.iSize;
  if (iX == g_tOpts.iWidth - 1) iBeam = (3 * g_tOpts.iSize + 1) - iY;
  if (iY == 0)                  iBeam = (4 * g_tOpts.iSize + 1) - iX;

  return iBeam;
}

/*******************************************************************************
 * Name:  lookAhead
 * Purpose: Look up content of cells ahead in walking direction.
 *******************************************************************************/
int lookAhead(int iCell, int iDirection) {
  int iFront      = 0;
  int iFrontLeft  = 0;
  int iFrontRight = 0;

  //           UP 1
  //            ^
  //            |
  // LEFT 2 <---+---> 4 RIGHT
  //            |
  //            V
  //         DOWN 3

  if (iDirection == DIR_UP) {
    iFront      = g_paiGrid[iCell - g_tOpts.iWidth];
    iFrontLeft  = g_paiGrid[iCell - g_tOpts.iWidth - 1];
    iFrontRight = g_paiGrid[iCell - g_tOpts.iWidth + 1];
  }
  if (iDirection == DIR_LEFT) {
    iFront      = g_paiGrid[iCell - 1];
    iFrontLeft  = g_paiGrid[iCell - 1 + g_tOpts.iWidth];
    iFrontRight = g_paiGrid[iCell - 1 - g_tOpts.iWidth];
  }
  if (iDirection == DIR_DOWN) {
    iFront      = g_paiGrid[iCell + g_tOpts.iWidth];
    iFrontLeft  = g_paiGrid[iCell + g_tOpts.iWidth + 1];
    iFrontRight = g_paiGrid[iCell + g_tOpts.iWidth - 1];
  }
  if (iDirection == DIR_RIGHT) {
    iFront      = g_paiGrid[iCell + 1];
    iFrontLeft  = g_paiGrid[iCell + 1 - g_tOpts.iWidth];
    iFrontRight = g_paiGrid[iCell + 1 + g_tOpts.iWidth];
  }

  if (iFront      == CELL_ATOM) return ATOM_CENTER;
  if (iFrontLeft  == CELL_ATOM) return ATOM_LEFT;
  if (iFrontRight == CELL_ATOM) return ATOM_RIGHT;

  return ATOM_NONE;
}

/*******************************************************************************
 * Name:  turnBeam
 * Purpose: Turn beam's direction according atoms and correct under-/overflows.
 *******************************************************************************/
int turnBeam(int iAtom, int iDirection) {
  // Turn right.
  if (iAtom == ATOM_LEFT)
    if (--iDirection == 0)
      iDirection = 4;

  // Turn left.
  if (iAtom == ATOM_RIGHT)
    if (++iDirection == 5)
      iDirection = 1;

  return iDirection;
}

/*******************************************************************************
 * Name:  goAhead
 * Purpose: Go one cell in the walking direction.
 *******************************************************************************/
int goAhead(int iCell, int iDirection) {
  if (iDirection == DIR_UP)    return iCell - g_tOpts.iWidth;
  if (iDirection == DIR_LEFT)  return iCell - 1;
  if (iDirection == DIR_DOWN)  return iCell + g_tOpts.iWidth;
  if (iDirection == DIR_RIGHT) return iCell + 1;

  return -1;
}

/*******************************************************************************
 * Name:  walkGrid
 * Purpose: Walks the beam across the board.
 *******************************************************************************/
int walkGrid(int iEntryNo, int iDirection) {
  int iAtom = 0;
  int iCell = iEntryNo;

  // Infinit loop, will stop via return;
  while (1) {
    iAtom = lookAhead(iCell, iDirection);

    // Beam was absorbed by an atom, done!
    if (iAtom == ATOM_CENTER) return 0;

    // Turn as long as atoms are on front sides.
    while (iAtom == ATOM_LEFT || iAtom == ATOM_RIGHT) {
      iDirection =  turnBeam(iAtom, iDirection);
      iAtom      = lookAhead(iCell, iDirection);
      // Still at border? Done!
      if (g_paiGrid[iCell] == CELL_BORDER) return iCell;
    }

    // A step ahead without an atom in the way.
    iCell = goAhead(iCell, iDirection);

    // At border again? Done!
    if (g_paiGrid[iCell] == CELL_BORDER) return iCell;
  }
}

/*******************************************************************************
 * Name:  getAtomAnswers
 * Purpose: Retrieves atom guesses from user and prints if entered correctly.
 *******************************************************************************/
void getAtomAnswers(void) {
  int  iCell = 0;
  int  iX    = 0;
  int  iY    = 0;

  printf("\n");
  printf("Enter coordinates of each Atom as "
         "y (down) and x (right) (each from 1 to %d)\n\n",
         g_tOpts.iSize);

  for (int i = 0; i < g_tOpts.iAtomNo; ++i) {
    printf("Atom %d of %d\n", i + 1, g_tOpts.iAtomNo);
    printf("y: "); scanf("%20d", &iY);
    printf("x: "); scanf("%20d", &iX);

    // Range check.
    if (iY< 1 || iY > g_tOpts.iSize ||
        iX< 1 || iX > g_tOpts.iSize) {
      printf("A coordinate is out of range, try again.\n");
      --i;
      continue;
    }

    printf("you entered down %i and right %i\n", iY, iX);

    cellFromXY(&iCell, iX, iY);

    if (g_paiGrid[iCell] == CELL_ATOM) {
      printf("Atom Found\n");
    }
    else {
      printf("Atom not found\n");
      ++g_tScore.iMissedAtoms;
    }
    printf("\n");
  }
}

/*******************************************************************************
 * Name:  printScore
 * Purpose: Prints final score.
 *******************************************************************************/
void printScore(void) {
  printf("Final score:\n");
  printf("-------------\n");
  printf("Missed Atoms    %3d x %3d = %3d\n",
         g_tScore.iMissedAtoms,
         SCORE_ATOM,
         SCORE_ATOM * g_tScore.iMissedAtoms);
  printf("Exited beams    %3d x %3d = %3d\n",
         g_tScore.iExited,
         SCORE_EXIT,
         SCORE_EXIT * g_tScore.iExited);
  printf("Reflected beams %3d x %3d = %3d\n",
         g_tScore.iReflected,
         SCORE_REFLECTED,
         SCORE_REFLECTED * g_tScore.iReflected);
  printf("Absorbed beams  %3d x %3d = %3d\n",
         g_tScore.iAbsorbed,
         SCORE_ABSORBED ,
         SCORE_ABSORBED * g_tScore.iAbsorbed);
  printf("----------------------------------\n");
  printf("Sum total                 = %3d\n",
         SCORE_ATOM      * g_tScore.iMissedAtoms +
         SCORE_EXIT      * g_tScore.iExited      +
         SCORE_REFLECTED * g_tScore.iReflected   +
         SCORE_ABSORBED  * g_tScore.iAbsorbed);
}


//******************************************************************************
//* main

int main(int argc, char *argv[]) {
  cstr csAnswer   = csNew("");
  int  iDirection = 0;
  int  iCellEntry = 0;
  int  iCellExit  = 0;
  int  iNodeExit  = 0;
  int  bEndOfLoop = 0;

  // Save program's name.underlined
  g_csMename = csNew("");
  getMename(&g_csMename, argv[0]);

  // Get options and dispatch errors, if any.
  getOptions(argc, argv);

  printIntro();
  createBoard();

  // Make sure to print the board at least once prior game play, here or in the
  // while loop.
  if (!g_tOpts.bPrtBrd)
    printBoard(BOARD_NEUTRAL);

  while (!bEndOfLoop) {
    if (g_tOpts.bPrtBrd)
      printBoard(BOARD_NEUTRAL);
    else
      printf("\n");

    csAnswer = getEntryNode(&iCellEntry, &iDirection);

    if (csAnswer.len == 0)  {
      printf("Not a number or command ...\n");
      continue;
    }

    if (csAnswer.cStr[0] == 'e' ||
        csAnswer.cStr[0] == 'E' ||
        csAnswer.cStr[0] == 'f' ||
        csAnswer.cStr[0] == 'F') {
      bEndOfLoop = 1;
      continue;
    }
    if (csAnswer.cStr[0] == 'q' ||
        csAnswer.cStr[0] == 'Q') {
      printf("Bye then ...\n");
      exit(0);
    }
    if (csAnswer.cStr[0] == 'b' ||
        csAnswer.cStr[0] == 'B') {
      printBoard(BOARD_NEUTRAL);
      continue;
    }

    if (iCellEntry == -1)  {
      printf("Beam out of bounds ...\n");
      continue;
    }

    iCellExit = walkGrid(iCellEntry, iDirection);

    printf("Beam ");

    if (iCellExit == iCellEntry) {
      printf("was reflected\n");
      ++g_tScore.iReflected;
      continue;
    }

    if (iCellExit == 0) {
      printf("was absorbed\n");
      ++g_tScore.iAbsorbed;
      continue;
    }

    iNodeExit = getExitNode(iCellExit);
    printf("exited at %d\n", iNodeExit);
    ++g_tScore.iExited;
  }

  getAtomAnswers();
  printBoard(BOARD_SOLUTION);
  printScore();

  // Free all used memory, prior end of program.
  csFree(&csAnswer);
  daFreeEx(g_tArgs, cStr);
  free(g_paiGrid);

  return ERR_NOERR;
}
