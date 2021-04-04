/*************************************************************/
/*                                                           */
/* Ed Trice's 6-piece perfect play databases				 */
/* Adapted for Jonathan Kreuzer's GUI CHECKERS program		 */
/*                                                           */
/*************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "moveGen.h"
#include "endgameDatabase.h"
#include "engine.h"

extern CheckerboardInterface checkerBoard;

/*************************************************************/
/*                                                           */
/* Info for using Ed Trice's 6-piece perfect play databases */
/*                                                           */
/*************************************************************/

#define DB_UNKNOWN_THAT_FITS_INTO_8_BITS	(255)
#define DB_DRAW_THAT_FITS_INTO_8_BITS		(254)
							
/*******************************************************/
/*                                                     */
/* 2 piece Indexing Function Constants                 */
/*                                                     */
/*******************************************************/

#define 	k_WR 1

#define 	k_wR 2
#define 	k_Wr 3

#define 	k_wr 4

/*******************************************************/
/*                                                     */
/* 3 piece Indexing Function Constants                 */
/*                                                     */
/*******************************************************/

#define		 k_WWR 1
#define		 k_RRW 2 

#define		 k_WwR 3
#define		 k_RrW 4

#define		 k_wwR 5
#define		 k_rrW 6

#define		 k_RRw 7
#define		 k_WWr 8

#define		 k_Wwr 9
#define		 k_Rrw 10

#define		 k_wwr 11
#define		 k_rrw 12

/*******************************************************/
/*                                                     */
/* 4 piece Indexing Function Constants                 */
/*                                                     */
/*******************************************************/

#define		 k_WWRR 1

#define		 k_WwRR 2
#define		 k_WWRr 3

#define		 k_WwRr 4 

#define		 k_wwRR 5
#define		 k_WWrr 6

#define		 k_wwRr 7 
#define		 k_Wwrr 8 

#define		 k_wwrr 9  

#define		 k_WWWR 10
#define		 k_RRRW 11

#define		 k_WWwR 12
#define		 k_RRrW 13

#define		 k_WwwR 14
#define		 k_RrrW 15

#define		 k_wwwR 16
#define		 k_rrrW 17

#define		 k_WWWr 18
#define		 k_RRRw 19

#define		 k_WWwr 20
#define		 k_RRrw 21

#define		 k_Wwwr 22
#define		 k_Rrrw 23

#define		 k_wwwr 24
#define		 k_rrrw 25

/*******************************************************/
/*                                                     */
/* 5 piece Indexing Function Constants                 */
/*                                                     */
/*******************************************************/

#define		 k_WWWRR 1	
#define		 k_RRRWW 2	

#define		 k_RRRWw 3	
#define		 k_WWWRr 4 

#define		 k_RRRww 5	
#define		 k_WWWrr 6  
			  
#define		 k_WWwRR 7  
#define		 k_RRrWW 8	

#define		 k_RRrWw 9   
#define		 k_WWwRr 10  

#define		 k_RRrww 11 
#define		 k_WWwrr 12 
			  
#define		 k_WwwRR 13 
#define		 k_RrrWW 14	

#define		 k_RrrWw 15 
#define		 k_WwwRr 16 

#define		 k_Rrrww 17 
#define		 k_Wwwrr 18 

#define		 k_wwwRR 19 
#define		 k_rrrWW 20	

#define		 k_rrrWw 21 
#define		 k_wwwRr 22 

#define		 k_rrrww 23 
#define		 k_wwwrr 24 

/*******************************************************/
/*                                                     */
/* 6 piece Indexing Function Constants                 */
/*                                                     */
/*******************************************************/

#define		 k_WWWRRR 1

#define		 k_WWwRRR 2   
#define		 k_WWWRRr 3

#define		 k_WwwRRR 4
#define		 k_WWWRrr 5

#define		 k_wwwRRR 6
#define		 k_WWWrrr 7

#define		 k_WWwRRr 8

#define		 k_WwwRRr 9
#define		 k_WWwRrr 10

#define		 k_wwwRRr 11
#define		 k_WWwrrr 12

#define		 k_WwwRrr 13

#define		 k_wwwRrr 14
#define		 k_Wwwrrr 15

#define		 k_wwwrrr 16

unsigned short g_piece_counts_to_local_slice[4][4][4][4];

/*********************************************************************/
/*                                                                   */
/* General Indexing Function Combinatoric Macros                     */
/* ---------------------------------------------                     */
/*                                                                   */
/* Given there are N identical pieces of the same color, how many    */
/* ways can they be arranged uniquely on a game board, independent   */
/* of the size of the board?                                         */
/*                                                                   */
/* The answer involves producing a polynomial expansion of the form  */
/*                                                                   */
/* a + (b-1)(b-2)/2! + (c-1)(c-2)(c-3)/3! + .. (z-1)(z-2)..(z-N)/N!  */
/*                                                                   */
/*********************************************************************/

#define _2_same_pieces_subindex(a,b) 		 ((a) + ((((b)-1)*((b)-2))/2))
#define _3_same_pieces_subindex(a,b,c)		 ((a) + ((((b)-1)*((b)-2))/2) + ((((c)-1)*((c)-2)*((c)-3))/6))
#define _4_same_pieces_subindex(a,b,c,d)	 ((a) + ((((b)-1)*((b)-2))/2) + ((((c)-1)*((c)-2)*((c)-3))/6) + ((((d)-1)*((d)-2)*((d)-3)*((d)-4))/24))	


/*******************************************************/
/*                                                     */
/* 2 piece Indexing Function Macros                    */
/*                                                     */
/*******************************************************/
#define _1K0C_AGAINST_1k0c_index(a,b)       ((((a)-1) * 31) + (b))
#define _1K0C_AGAINST_0k1c_index(a,b)       ((((a)-1) * 31) + (b))

/*******************************************************/
/*                                                     */
/* 3 piece Indexing Function Macros                    */
/*                                                     */
/*******************************************************/

#define _2K0C_AGAINST_1k0c_index(a,b,c)					((((_2_same_pieces_subindex((a),(b))) - 1) * 30) + (c))
#define _2K0C_AGAINST_0k1c_index(a,b,c)					((((a) - 1) * 465) + (_2_same_pieces_subindex((b),(c))))

#define _1K1C_AGAINST_1k0c_index(a,b,c)					((((a)-1) * 930) + (((b)-1) * 30) + (c))
#define _1K1C_AGAINST_0k1c_index(Q,c)					((((Q)-1) * 30) + (c))

#define _0K2C_AGAINST_1k0c_index(a,b,c)					((((_2_same_pieces_subindex((a),(b))) - 1) * 30) + (c))

/*******************************************************/
/*                                                     */
/* 4 piece Indexing Function Macros                    */
/*                                                     */
/*******************************************************/

#define _2K0C_AGAINST_2k0c_index(a,b,c,d)		((((_2_same_pieces_subindex((a),(b))) - 1) * 435) + (_2_same_pieces_subindex((c),(d))))
#define _2K0C_AGAINST_1k1c_index(a,b,c,d)		((((a)-1) * 13485) + (((_2_same_pieces_subindex((b),(c))) - 1) * 29) + (d))
#define _2K0C_AGAINST_0k2c_index(a,b,c,d)		((((_2_same_pieces_subindex((a),(b))) - 1) * 435) + (_2_same_pieces_subindex((c),(d))))

#define _1K1C_AGAINST_1k1c_index(Q,c,d) 		((((Q)-1) * 870) + (((c)-1) * 29) + (d))
#define _1K1C_AGAINST_0k2c_index(Q,d) 			((((Q)-1) * 29) + (d))

#define _3K0C_AGAINST_1k0c_index(a,b,c,d)		((((a)-1) * 4495) + (_3_same_pieces_subindex((b),(c),(d))))
#define _3K0C_AGAINST_0k1c_index(a,b,c,d)		((((a)-1) * 4495) + (_3_same_pieces_subindex((b),(c),(d))))

#define _2K1C_AGAINST_1k0c_index(a,b,c,d)     	((((a)-1) * 13485) + (((b)-1) * 435) + ((_2_same_pieces_subindex((c),(d)))))
#define _2K1C_AGAINST_0k1c_index(Q,c,d)  		((((Q)-1) * 435) + ((_2_same_pieces_subindex((c),(d)))))

#define _1K2C_AGAINST_1k0c_index(a,b,c,d)     	((((_2_same_pieces_subindex((a),(b))) -1) * 870) + (((c)-1) * 29) + (d))
#define _1K2C_AGAINST_0k1c_index(Q,d) 			((((Q)-1) * 29) + (d))

#define _0K3C_AGAINST_1k0c_index(a,b,c,d)       (((_3_same_pieces_subindex((a),(b),(c)) - 1) * 29) + (d))

/*******************************************************/
/*                                                     */
/* 5 piece Indexing Function Macros                    */
/*                                                     */
/*******************************************************/
#define _3K0C_AGAINST_2k0c_index(a,b,c,d,e)		((((_3_same_pieces_subindex((a),(b),(c))) - 1) * 406) + (_2_same_pieces_subindex((d),(e))))
#define _3K0C_AGAINST_1k1c_index(a,b,c,d,e)		((((a)-1)*125860) +  ((_3_same_pieces_subindex((b),(c),(d)) - 1) * 28) + (e))
#define _3K0C_AGAINST_0k2c_index(a,b,c,d,e)		((((_2_same_pieces_subindex((a),(b))) - 1) * 4060) + (_3_same_pieces_subindex((c),(d),(e))))

#define _2K1C_AGAINST_2k0c(a,b,c,d,e)			((((a)-1)*188790) + ((_2_same_pieces_subindex((b),(c)) - 1)*406) + _2_same_pieces_subindex((d),(e)))
#define _2K1C_AGAINST_1k1c(Q,b,c,d)				((((Q)-1)*12180) + (((b)-1)*406) + (_2_same_pieces_subindex((c),(d))))
#define _2K1C_AGAINST_0k2c(Q,d,e)				((((Q)-1)*406) + (_2_same_pieces_subindex((d),(e))))

#define _1K2C_AGAINST_2k0c(a,b,c,d,e)			((((_2_same_pieces_subindex((a),(b))) - 1) * 12180) + (((_2_same_pieces_subindex((c),(d))) - 1) * 28) + (e))				
#define _1K2C_AGAINST_1k1c(Q,d,e)				((((Q)-1)*812) + (((d)-1)*28) + (e))
#define _1K2C_AGAINST_0k2c(Q,e)					((((Q)-1) * 28) + (e))

#define _0K3C_AGAINST_2k0c_index(a,b,c,d,e)		((((_3_same_pieces_subindex((a),(b),(c))) - 1) * 406) + (_2_same_pieces_subindex((d),(e))))
#define _0K3C_AGAINST_1k1c(Q,e)					((((Q)-1) * 28) + (e))

/*******************************************************/
/*                                                     */
/* 6 piece Indexing Function Macros                    */
/*                                                     */
/*******************************************************/
#define _3K0C_AGAINST_3k0c_index(a,b,c,d,e,f)	((((_3_same_pieces_subindex((a),(b),(c))) - 1) * 3654) + (_3_same_pieces_subindex((d),(e),(f))))
#define _3K0C_AGAINST_2k1c_index(a,b,c,d,e,f)   (((a)-1) * 1699110 + (((_3_same_pieces_subindex((b),(c),(d))) - 1) * 378)  + (_2_same_pieces_subindex((e),(f))))
#define _3K0C_AGAINST_1k2c_index(a,b,c,d,e,f)   (((_2_same_pieces_subindex((a),(b)) - 1) * 109620)  +  (((_3_same_pieces_subindex((c),(d),(e))) - 1) * 27) + (f))
#define _3K0C_AGAINST_0k3c_index(a,b,c,d,e,f)	((((_3_same_pieces_subindex((a),(b),(c))) - 1) * 3654) + (_3_same_pieces_subindex((d),(e),(f))))

#define _2K1C_AGAINST_2k1c_index(Q,c,d,e,f)		((((Q)-1) * 164430) + (((_2_same_pieces_subindex((c),(d)))-1)*378) + (_2_same_pieces_subindex((e),(f))))
#define _1K2C_AGAINST_2k1c_index(Q,d,e,f)		((((Q)-1) * 10962) + (((_2_same_pieces_subindex((d),(e)))-1) * 27) + (f))
#define _1K2C_AGAINST_1k2c_index(Q,e,f)			((((Q)-1) * 756) + (((e) - 1) * 27) + (f))

#define _0K3C_AGAINST_2k1c_index(Q,e,f)		    ((((Q)-1) * 378) + (_2_same_pieces_subindex((e),(f))))
#define _0K3C_AGAINST_1k2c_index(Q,f)			((((Q)-1) * 27) + (f))


unsigned long get_02_piece_index_1K0C_AGAINST_1k0c(unsigned char wk1, unsigned char rk1)
{
	unsigned long index_function_value=0;
	unsigned char adjust_rk1_index=rk1;

	if(rk1 > wk1)
		adjust_rk1_index--;
					
	index_function_value = _1K0C_AGAINST_1k0c_index(wk1, adjust_rk1_index) - 1;
	return index_function_value;
}

unsigned long get_02_piece_index_1K0C_AGAINST_0k1c(unsigned char wk1, unsigned char rc1)
{
	unsigned long index_function_value = 0;
	unsigned char adjust_wk1_index = wk1;

	if(wk1 > rc1)
		adjust_wk1_index--;
					
	index_function_value = _1K0C_AGAINST_0k1c_index(rc1, adjust_wk1_index) - 1;
	return index_function_value;
}

unsigned long get_02_piece_index_0K1C_AGAINST_0k1c(unsigned char wc1, unsigned char rc1)
{
	unsigned long combined_checker_contribution_to_index;

	combined_checker_contribution_to_index = 0;
	
	if(rc1 < 5)
		{
			combined_checker_contribution_to_index = (28 * (rc1 - 1)) + (wc1 - 4);
		}
	else
		{
			if(wc1 > rc1)
				combined_checker_contribution_to_index = (112) + (rc1 - 5)*27 + (wc1 - 5);
			else
				combined_checker_contribution_to_index = (112) + (rc1 - 5)*27 + (wc1 - 4);
		}
					

	return combined_checker_contribution_to_index - 1;
}

unsigned long get_02_piece_index_for_slice(unsigned char which_slice, unsigned char square1, unsigned char square2)
{
	unsigned long index_function_value;

	index_function_value = 0;
	
	switch(which_slice)
	{
		case k_WR:
			index_function_value = get_02_piece_index_1K0C_AGAINST_1k0c(square1, square2);
		break;
				
		case k_Wr:
			index_function_value = get_02_piece_index_1K0C_AGAINST_0k1c(square1, square2);
		break;
		
		case k_wR:
			index_function_value = get_02_piece_index_1K0C_AGAINST_0k1c(33-square2, 33-square1);
		break;
		
		case k_wr:
			index_function_value = get_02_piece_index_0K1C_AGAINST_0k1c(square1, square2);
		break;
		
		default:
			/*open_error_report();
			fprintf(g_error_info, "get_02_piece_index_for_slice: slice %d has no case statement.\n", which_slice);
			close_error_report();*/
			exit(0);
		break;
		
	}
	
	return index_function_value;
}


unsigned long get_03_piece_index_2K0C_AGAINST_1k0c(unsigned char wk1, unsigned char wk2, unsigned char rk1)
{
	unsigned long   index_function_value=0;
	unsigned char 	adjust_rk1_index=rk1;

	if(rk1 > wk1)
		adjust_rk1_index--;
		
	if(rk1 > wk2)
		adjust_rk1_index--;
			
	index_function_value = _2K0C_AGAINST_1k0c_index(wk1, wk2, adjust_rk1_index) - 1;
	return index_function_value;
}

unsigned long get_03_piece_index_2K0C_AGAINST_0k1c(unsigned char wk1, unsigned char wk2, unsigned char rc1)
{
	unsigned long   index_function_value=0;
	unsigned char 	adjust_wk1=wk1, adjust_wk2=wk2;

	if(wk1 > rc1)
		adjust_wk1--;
		
	if(wk2 > rc1)
		adjust_wk2--;
			
	index_function_value = _2K0C_AGAINST_0k1c_index(rc1, adjust_wk1, adjust_wk2) - 1;
	return index_function_value;
}

unsigned long get_03_piece_index_1K1C_AGAINST_1k0c(unsigned char wk1, unsigned char wc1, unsigned char rk1)
{
	unsigned long   index_function_value=0;
	unsigned char 	adjust_wc1=wc1-4, adjust_wk1=wk1, adjust_rk1=rk1;

	if(wk1 > wc1)
		adjust_wk1--;
		
	if(wk1 > rk1)
		adjust_wk1--;
		
	if(rk1 > wc1)
		adjust_rk1--;
			
	index_function_value = _1K1C_AGAINST_1k0c_index(adjust_wc1, adjust_rk1, adjust_wk1) - 1;
	return index_function_value;
}

unsigned long get_03_piece_index_1K1C_AGAINST_0k1c(unsigned char wk1, unsigned char wc1, unsigned char rc1)
{
	unsigned long   index_function_value=0, combined_checker_contribution_to_index;
	unsigned char 	adjust_wk1=wk1;

	if(wk1 > wc1)
		adjust_wk1--;
		
	if(wk1 > rc1)
		adjust_wk1--;
					
			
	combined_checker_contribution_to_index = 0;
	
	if(rc1 < 5)
		{
			combined_checker_contribution_to_index = (28 * (rc1 - 1)) + (wc1 - 4);
		}
	else
		{
			if(wc1 > rc1)
				combined_checker_contribution_to_index = (112) + (rc1 - 5)*27 + (wc1 - 5);
			else
				combined_checker_contribution_to_index = (112) + (rc1 - 5)*27 + (wc1 - 4);
		}
						
	index_function_value = _1K1C_AGAINST_0k1c_index(combined_checker_contribution_to_index, adjust_wk1) - 1;
	return index_function_value;
}


unsigned long get_03_piece_index_0K2C_AGAINST_1k0c(unsigned char wc1, unsigned char wc2, unsigned char rk1)
{
	unsigned long   index_function_value=0;
	unsigned char 	adjust_rk1_index=rk1, adjust_wc1=wc1-4, adjust_wc2=wc2-4;

	if(rk1 > wc1)
		adjust_rk1_index--;
		
	if(rk1 > wc2)
		adjust_rk1_index--;
			
	index_function_value = _0K2C_AGAINST_1k0c_index(adjust_wc1, adjust_wc2, adjust_rk1_index) - 1;
	return index_function_value;
}


unsigned long get_03_piece_index_0K2C_AGAINST_0k1c(unsigned char wc1, unsigned char wc2, unsigned char rc1)
{
	unsigned long   index_function_value=0, combined_checker_contribution_to_index;
	unsigned char 	adjust_wc1_index=wc1, adjust_wc2_index=wc2;

	combined_checker_contribution_to_index = 0;
	
	if(rc1 < 5)
		{
			adjust_wc1_index -= 4;
			adjust_wc2_index -= 4;
			
			combined_checker_contribution_to_index = (378 * (rc1 - 1)) + _2_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index);
		}
	else
		{
			if(wc1 > rc1) 
				adjust_wc1_index -= 5;
			else
				adjust_wc1_index -= 4;

			if(wc2 > rc1) 
				adjust_wc2_index -= 5;
			else
				adjust_wc2_index -= 4;

			
			combined_checker_contribution_to_index = (1512) + (rc1 - 5)*351 + _2_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index);

		}
			
	index_function_value = combined_checker_contribution_to_index - 1;
	return index_function_value;
}

unsigned long get_03_piece_index_for_slice(unsigned char which_slice, unsigned char square1, unsigned char square2, unsigned char square3)
{
	unsigned long index_function_value;
	
	index_function_value = 0;
	
	switch(which_slice)
	{
		case k_WWR:
			index_function_value = get_03_piece_index_2K0C_AGAINST_1k0c(square1, square2, square3);
		break;
		case k_RRW:
			index_function_value = get_03_piece_index_2K0C_AGAINST_1k0c(33-square2, 33-square1, 33-square3);
		break;
		
		case k_WWr:
			index_function_value = get_03_piece_index_2K0C_AGAINST_0k1c(square1, square2, square3);
		break;
		case k_RRw:
			index_function_value = get_03_piece_index_2K0C_AGAINST_0k1c(33-square2, 33-square1, 33-square3);
		break;
		
		
		case k_WwR:
			index_function_value = get_03_piece_index_1K1C_AGAINST_1k0c(square1, square2, square3);
		break;
		case k_RrW:
			index_function_value = get_03_piece_index_1K1C_AGAINST_1k0c(33-square1, 33-square2, 33-square3);
		break;
		
		
		case k_Wwr:
			index_function_value = get_03_piece_index_1K1C_AGAINST_0k1c(square1, square2, square3);
		break;
		case k_Rrw:
			index_function_value = get_03_piece_index_1K1C_AGAINST_0k1c(33-square1, 33-square2, 33-square3);
		break;
		
		
		case k_wwR:
			index_function_value = get_03_piece_index_0K2C_AGAINST_1k0c(square1, square2, square3);
		break;
		case k_rrW:
			index_function_value = get_03_piece_index_0K2C_AGAINST_1k0c(33-square2, 33-square1, 33-square3);
		break;
		
		case k_wwr:
			index_function_value = get_03_piece_index_0K2C_AGAINST_0k1c(square1, square2, square3);
		break;
		case k_rrw:
			index_function_value = get_03_piece_index_0K2C_AGAINST_0k1c(33-square2, 33-square1, 33-square3);
		break;
		
		default:
			/*open_error_report();
			fprintf(g_error_info, "get_03_piece_index_for_slice: slice %d has no case statement.\n", which_slice);
			close_error_report();*/
			exit(0);
		break;
		
	}
	
	return index_function_value;
}

unsigned long get_04_piece_index_2K0C_AGAINST_2k0c(unsigned char wk1, unsigned char wk2, unsigned char bk1, unsigned char bk2)
{
	unsigned long   index_function_value=0;
	unsigned char 	adjust_bk1_index=bk1,
					adjust_bk2_index=bk2;

	if(bk1 > wk1)
		adjust_bk1_index--;
		
	if(bk1 > wk2)
		adjust_bk1_index--;
	
	if(bk2 > wk1)
		adjust_bk2_index--;
		
	if(bk2 > wk2)
		adjust_bk2_index--;
		
	index_function_value = _2K0C_AGAINST_2k0c_index(wk1, wk2, adjust_bk1_index, adjust_bk2_index) - 1;
	return index_function_value;
}

unsigned long get_04_piece_index_2K0C_AGAINST_1k1c(unsigned char wk1, unsigned char wk2, unsigned char bk, unsigned char bc)
{
	unsigned long   index_function_value=0;
	unsigned char 	adjust_wk1_index,
					adjust_wk2_index,
					adjust_bk_index;
					
	/* order placed on board: bc, wk1, wk2, bk */
		
	adjust_wk1_index = wk1;
	adjust_wk2_index = wk2;
	adjust_bk_index = bk;
					
	if(wk1 > bc)	
		adjust_wk1_index--;
		
	if(wk2 > bc)
		adjust_wk2_index--;
		
	if(bk > bc)
		adjust_bk_index--;
		
	if(bk > wk1)
		adjust_bk_index--;
   
   	if(bk > wk2)
		adjust_bk_index--;

	index_function_value = (_2K0C_AGAINST_1k1c_index(bc, adjust_wk1_index, adjust_wk2_index, adjust_bk_index)) - 1;
	
	return index_function_value;

}

unsigned long get_04_piece_index_2K0C_AGAINST_0k2c(unsigned char wk1, unsigned char wk2, unsigned char bc1, unsigned char bc2)
{
	unsigned long   index_function_value=0;
	unsigned char 	adjust_wk1_index=wk1,
					adjust_wk2_index=wk2;

	if(wk1 > bc1)
		adjust_wk1_index--;
		
	if(wk1 > bc2)
		adjust_wk1_index--;
	
	if(wk2 > bc1)
		adjust_wk2_index--;
		
	if(wk2 > bc2)
		adjust_wk2_index--;
		
	index_function_value = _2K0C_AGAINST_0k2c_index(bc1, bc2, adjust_wk1_index, adjust_wk2_index) - 1;
	return index_function_value;
}



unsigned long get_04_piece_index_1K1C_AGAINST_1k1c(unsigned char wk, unsigned char wc, unsigned char bk, unsigned char bc)
{
	unsigned long   index_function_value=0,
	                 combined_checker_contribution_to_index=0;
	                 
	unsigned char 	adjust_bk_index=bk,
					adjust_wk_index=wk;

		
	if(bc < 5)
		{
			combined_checker_contribution_to_index = (28 * (bc - 1)) + (wc - 4);
		}
	else
		{
			if(wc > bc)
				combined_checker_contribution_to_index = (112) + (bc - 5)*27 + (wc - 5);
			else
				combined_checker_contribution_to_index = (112) + (bc - 5)*27 + (wc - 4);
		}

	
	if(bk > bc)		
		adjust_bk_index--;
		
	if(bk > wc)		
		adjust_bk_index--;
		
	if(wk > bc)		
		adjust_wk_index--;
		
	if(wk > wc)		
		adjust_wk_index--;
		
	if(wk > bk)		
		adjust_wk_index--;
		
	index_function_value = _1K1C_AGAINST_1k1c_index(combined_checker_contribution_to_index, adjust_bk_index, adjust_wk_index) - 1;
	return index_function_value;

}

unsigned long get_04_piece_index_1K1C_AGAINST_0k2c(unsigned char wk, unsigned char wc, unsigned char bc1, unsigned char bc2)
{

	unsigned long   index_function_value=0,
	                 combined_checker_contribution_to_index=0;
	                 
	unsigned char 	adjust_wk_index;

	/****************************************/
	/* completely rewritten 12-26-2004      */
	/* Rest in peace, Reggie White          */
	/* order of placement: wc, bc1, bc2, wk */
	/****************************************/

	combined_checker_contribution_to_index = get_03_piece_index_0K2C_AGAINST_0k1c(33 - bc2, 33 - bc1, 33 - wc);
	
	adjust_wk_index = wk;
	
	if(wk > bc2)
		adjust_wk_index--;
		
	if(wk > bc1)
		adjust_wk_index--;

	if(wk > wc)
		adjust_wk_index--;
						
		
	index_function_value = (combined_checker_contribution_to_index * 29) + adjust_wk_index - 1;
	return index_function_value;

}

unsigned long get_04_piece_index_0K2C_AGAINST_0k2c(unsigned char wc1, unsigned char wc2, unsigned char bc1, unsigned char bc2)
{
	unsigned long combined_checker_contribution_to_index = 0;
	unsigned char adjust_wc1_index;
	unsigned char adjust_wc2_index;
	unsigned char adjust_bc1_index;
	unsigned char adjust_bc2_index;
	
	adjust_wc1_index = wc1 - 4;
	adjust_wc2_index = wc2 - 4;
	
	if((wc1 > bc1) && (bc1 >=5))
		adjust_wc1_index--;
		
	if((wc1 > bc2) && (bc2 >=5))
		adjust_wc1_index--;

	
	if((wc2 > bc1) && (bc1 >=5))
		adjust_wc2_index--;
		
	if((wc2 > bc2) && (bc2 >=5))
		adjust_wc2_index--;
	
	
	if((bc1 < 5) && (bc2 < 5))
		{
			combined_checker_contribution_to_index = ((_2_same_pieces_subindex(bc1, bc2) - 1) * 378) + ((_2_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index)));
		}
	else
	if((bc1 < 5) && (bc2 > 4))
		{
			combined_checker_contribution_to_index = (2268) + ((((bc1 - 1) * 24) + (bc2 - 5)) * 351) + ((_2_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index)));
		}
	else
		{
			adjust_bc1_index = bc1 - 4;
			adjust_bc2_index = bc2 - 4;
						
			combined_checker_contribution_to_index = (33696 + 2268) + ((_2_same_pieces_subindex(adjust_bc1_index, adjust_bc2_index) - 1) * 325) + ((_2_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index)));
		}
		
	return (combined_checker_contribution_to_index - 1);
}

unsigned long get_04_piece_index_3K0C_AGAINST_1k0c(unsigned char wk1, unsigned char wk2, unsigned char wk3, unsigned char bk)
{
	unsigned long index_function_value = 0;
	
	unsigned char adjust_wk1_index = wk1;
	unsigned char adjust_wk2_index = wk2;
	unsigned char adjust_wk3_index = wk3;
	
	if(wk1 > bk)
		adjust_wk1_index--;
	
	if(wk2 > bk)
		adjust_wk2_index--;

	if(wk3 > bk)
		adjust_wk3_index--;
	
 	index_function_value = _3K0C_AGAINST_1k0c_index(bk, adjust_wk1_index, adjust_wk2_index, adjust_wk3_index) - 1;
 	
 	return index_function_value;
}

unsigned long get_04_piece_index_3K0C_AGAINST_0k1c(unsigned char wk1, unsigned char wk2, unsigned char wk3, unsigned char bc)
{
	unsigned long index_function_value = 0;
	
	unsigned char adjust_wk1_index = wk1;
	unsigned char adjust_wk2_index = wk2;
	unsigned char adjust_wk3_index = wk3;
	
	if(wk1 > bc)
		adjust_wk1_index--;
	
	if(wk2 > bc)
		adjust_wk2_index--;

	if(wk3 > bc)
		adjust_wk3_index--;
	
 	index_function_value = _3K0C_AGAINST_0k1c_index(bc, adjust_wk1_index, adjust_wk2_index, adjust_wk3_index) - 1;
 	
 	return index_function_value;
}

unsigned long get_04_piece_index_2K1C_AGAINST_1k0c(unsigned char wk1, unsigned char wk2, unsigned char wc, unsigned char bk)
{
	unsigned long index_function_value = 0;
	
	unsigned char adjust_wk1_index = wk1;
	unsigned char adjust_wk2_index = wk2;
	unsigned char adjust_bk_index = bk;
	

	if(bk > wc)	
		adjust_bk_index--;
		
	if(wk1 > wc)	
		adjust_wk1_index--;
		
	if(wk2 > wc)	
		adjust_wk2_index--;
		
	if(wk1 > bk)	
		adjust_wk1_index--;
		
	if(wk2 > bk)	
		adjust_wk2_index--;

 	index_function_value = _2K1C_AGAINST_1k0c_index(wc-4, adjust_bk_index, adjust_wk1_index, adjust_wk2_index) - 1;
 	
 	return index_function_value;
}

unsigned long get_04_piece_index_2K1C_AGAINST_0k1c(unsigned char wk1, unsigned char wk2, unsigned char wc, unsigned char bc)
{
	unsigned long index_function_value = 0,
	              combined_checker_contribution_to_index = 0;
	
	unsigned char adjust_wk1_index = wk1;
	unsigned char adjust_wk2_index = wk2;
	

	if(bc < 5)
		{
			combined_checker_contribution_to_index = (28 * (bc - 1)) + (wc - 4);
		}
	else
		{
			if(wc > bc)
				combined_checker_contribution_to_index = (112) + (bc - 5)*27 + (wc - 5);
			else
				combined_checker_contribution_to_index = (112) + (bc - 5)*27 + (wc - 4);
		}

	if(wk1 > wc)
		adjust_wk1_index--;
		
	if(wk1 > bc)
		adjust_wk1_index--;
		
	if(wk2 > wc)
		adjust_wk2_index--;
		
	if(wk2 > bc)
		adjust_wk2_index--;
		

 	index_function_value = _2K1C_AGAINST_0k1c_index(combined_checker_contribution_to_index, adjust_wk1_index, adjust_wk2_index) - 1;
 	
 	return index_function_value;
}

unsigned long get_04_piece_index_1K2C_AGAINST_1k0c(unsigned char wk, unsigned char wc1, unsigned char wc2, unsigned char bk)
{
	unsigned long index_function_value = 0;
	
	unsigned char adjust_wk_index = wk;
	unsigned char adjust_bk_index = bk;
	

	if(bk > wc1)
		adjust_bk_index--;
		
	if(bk > wc2)
		adjust_bk_index--;
		
	if(wk > wc1)
		adjust_wk_index--;
		
	if(wk > wc2)
		adjust_wk_index--;
		
	if(wk > bk)
		adjust_wk_index--;

 	index_function_value = _1K2C_AGAINST_1k0c_index(wc1-4, wc2-4, adjust_bk_index, adjust_wk_index) - 1;
 	
 	return index_function_value;
}

unsigned long get_04_piece_index_1K2C_AGAINST_0k1c(unsigned char wk, unsigned char wc1, unsigned char wc2, unsigned char bc)
{
	unsigned long index_function_value = 0, combined_checker_contribution_to_index = 0;
	
	unsigned char adjust_wk_index = wk;
	unsigned char adjust_wc1_index = wc1;
	unsigned char adjust_wc2_index = wc2;

	combined_checker_contribution_to_index = 0;
	
	if(bc < 5)
		{
			adjust_wc1_index -= 4;
			adjust_wc2_index -= 4;
			
			combined_checker_contribution_to_index = (378 * (bc - 1)) + _2_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index);
		}
	else
		{
			if(wc1 > bc) 
				adjust_wc1_index -= 5;
			else
				adjust_wc1_index -= 4;

			if(wc2 > bc) 
				adjust_wc2_index -= 5;
			else
				adjust_wc2_index -= 4;

			
			combined_checker_contribution_to_index = (1512) + (bc - 5)*351 + _2_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index);

		}

	if(wk > bc)
		adjust_wk_index--;
		
	if(wk > wc1)
		adjust_wk_index--;
		
	if(wk > wc2)
		adjust_wk_index--;

 	index_function_value = _1K2C_AGAINST_0k1c_index(combined_checker_contribution_to_index, adjust_wk_index) - 1;
 	
 	return index_function_value;
}

unsigned long get_04_piece_index_0K3C_AGAINST_1k0c(unsigned char wc1, unsigned char wc2, unsigned char wc3, unsigned char bk)
{
	unsigned long index_function_value = 0;
	
	unsigned char adjust_bk_index = bk;
	

	if(bk > wc1)
		adjust_bk_index--;
		
	if(bk > wc2)
		adjust_bk_index--;
		
	if(bk > wc3)
		adjust_bk_index--;

 	index_function_value = _0K3C_AGAINST_1k0c_index(wc1-4, wc2-4, wc3-4, adjust_bk_index) - 1;
 	
 	return index_function_value;
}

unsigned long get_04_piece_index_0K3C_AGAINST_0k1c(unsigned char wc1, unsigned char wc2, unsigned char wc3, unsigned char bc)
{
	unsigned char 	adjust_wc1_index,
					adjust_wc2_index,
					adjust_wc3_index;

	unsigned long 	combined_checker_contribution_to_index;
							
	adjust_wc1_index = wc1 - 4;
	adjust_wc2_index = wc2 - 4;
	adjust_wc3_index = wc3 - 4;
	
	
	if((wc1 > bc) && (bc >= 5))
		adjust_wc1_index--;
		
	if((wc2 > bc) && (bc >= 5))
		adjust_wc2_index--;
		
	if((wc3 > bc) && (bc >= 5))
		adjust_wc3_index--;
						
	combined_checker_contribution_to_index = 0;
		
	if(bc < 5)
		{
			combined_checker_contribution_to_index = ((bc - 1) * 3276) + (_3_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index, adjust_wc3_index));

		}
	else
		{
			combined_checker_contribution_to_index = (13104) + ((bc - 5) * 2925) + (_3_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index, adjust_wc3_index));
		}

		
	return ((combined_checker_contribution_to_index) - 1);

}

unsigned long get_04_piece_index_for_slice(unsigned char which_slice, unsigned char square1, unsigned char square2, unsigned char square3, unsigned char square4)
{
	unsigned long index_function_value;
	
	index_function_value = 0;
	
	switch(which_slice)
	{
		case k_WWRR:
			index_function_value = get_04_piece_index_2K0C_AGAINST_2k0c(square1, square2, square3, square4);
		break;
		
		case k_WWRr: 
			index_function_value = get_04_piece_index_2K0C_AGAINST_1k1c(square1, square2, square3, square4);
		break;
		
		case k_WWrr:
			index_function_value = get_04_piece_index_2K0C_AGAINST_0k2c(square1, square2, square3, square4);
		break;
		
		case k_WwRR: 
			index_function_value = get_04_piece_index_2K0C_AGAINST_1k1c(33-square4, 33-square3, 33-square1, 33-square2);
		break;
		
		case k_WwRr:
			index_function_value = get_04_piece_index_1K1C_AGAINST_1k1c(square1, square2, square3, square4);
		break;

		case k_Wwrr:
			index_function_value = get_04_piece_index_1K1C_AGAINST_0k2c(square1, square2, square3, square4);
		break;

		case k_wwRR:
			index_function_value = get_04_piece_index_2K0C_AGAINST_0k2c(33-square4, 33-square3, 33-square2, 33-square1);
		break;
				
		case k_wwRr:
			index_function_value = get_04_piece_index_1K1C_AGAINST_0k2c(33-square3, 33-square4, 33-square2, 33-square1);
		break;
				
		case k_wwrr:
			index_function_value = get_04_piece_index_0K2C_AGAINST_0k2c(square1, square2, square3, square4);
		break;
				
		/***** 3 versus 1 *****/
		case k_WWWR:
			index_function_value = get_04_piece_index_3K0C_AGAINST_1k0c(square1, square2, square3, square4);
		break;
		
		case k_RRRW:
			index_function_value = get_04_piece_index_3K0C_AGAINST_1k0c(33-square3, 33-square2, 33-square1, 33-square4);
		break;
		
		case k_WWWr:
			index_function_value = get_04_piece_index_3K0C_AGAINST_0k1c(square1, square2, square3, square4);
		break;
		
		case k_RRRw:
			index_function_value = get_04_piece_index_3K0C_AGAINST_0k1c(33-square3, 33-square2, 33-square1, 33-square4);
		break;
		
		case k_WWwR:
			index_function_value = get_04_piece_index_2K1C_AGAINST_1k0c(square1, square2, square3, square4);
		break;
		
		case k_RRrW:
			index_function_value = get_04_piece_index_2K1C_AGAINST_1k0c(33-square2, 33-square1, 33-square3, 33-square4);
		break;
		
		case k_WWwr:
			index_function_value = get_04_piece_index_2K1C_AGAINST_0k1c(square1, square2, square3, square4);
		break;
		
		case k_RRrw:
			index_function_value = get_04_piece_index_2K1C_AGAINST_0k1c(33-square2, 33-square1, 33-square3, 33-square4);
		break;
		
		case k_WwwR:
			index_function_value = get_04_piece_index_1K2C_AGAINST_1k0c(square1, square2, square3, square4);
		break;

		case k_RrrW:
			index_function_value = get_04_piece_index_1K2C_AGAINST_1k0c(33-square1, 33-square3, 33-square2, 33-square4);
		break;
		
		case k_Wwwr:
			index_function_value = get_04_piece_index_1K2C_AGAINST_0k1c(square1, square2, square3, square4);
		break;
		
		case k_Rrrw:
			index_function_value = get_04_piece_index_1K2C_AGAINST_0k1c(33-square1, 33-square3, 33-square2, 33-square4);
		break;
		
		case k_wwwR:
			index_function_value = get_04_piece_index_0K3C_AGAINST_1k0c(square1, square2, square3, square4);
		break;
		
		case k_rrrW:
			index_function_value = get_04_piece_index_0K3C_AGAINST_1k0c(33-square3, 33-square2, 33-square1, 33-square4);
		break;
		
		case k_wwwr:
			index_function_value = get_04_piece_index_0K3C_AGAINST_0k1c(square1, square2, square3, square4);
		break;
		
		case k_rrrw:
			index_function_value = get_04_piece_index_0K3C_AGAINST_0k1c(33-square3, 33-square2, 33-square1, 33-square4);
		break;
	}
	
	return index_function_value;
}


/*******************************************************/
/*                                                     */
/* 5 piece Indexing Function Procedures                */
/*                                                     */
/*******************************************************/
unsigned long get_05_piece_index_3K0C_AGAINST_2k0c(unsigned char wk1, unsigned char wk2, unsigned char wk3, unsigned char bk1, unsigned char bk2)
{
	unsigned char 	adjust_bk1_index,
					adjust_bk2_index;
					
	adjust_bk1_index = bk1;
	adjust_bk2_index = bk2;
	
	if(bk1 > wk1)
		adjust_bk1_index--;
		
	if(bk1 > wk2)
		adjust_bk1_index--;
		
	if(bk1 > wk3)
		adjust_bk1_index--;
		
	if(bk2 > wk1)
		adjust_bk2_index--;
		
	if(bk2 > wk2)
		adjust_bk2_index--;
		
	if(bk2 > wk3)
		adjust_bk2_index--;
		
	return (_3K0C_AGAINST_2k0c_index(wk1, wk2, wk3, adjust_bk1_index, adjust_bk2_index) - 1);

}

unsigned long get_05_piece_index_3K0C_AGAINST_1k1c(unsigned char wk1, unsigned char wk2, unsigned char wk3, unsigned char bk, unsigned char bc)
{
	unsigned char 	adjust_bk_index,
					adjust_wk1_index,
					adjust_wk2_index,
					adjust_wk3_index;
					
	adjust_bk_index = bk;
	
	adjust_wk1_index = wk1;
	adjust_wk2_index = wk2;
	adjust_wk3_index = wk3;
	
	if(wk1 > bc)
		adjust_wk1_index--;
		
	if(wk2 > bc)
		adjust_wk2_index--;

	if(wk3 > bc)
		adjust_wk3_index--;
		
	if(bk > bc)
		adjust_bk_index--;
	
	if(bk > wk1)
		adjust_bk_index--;
		
	if(bk > wk2)
		adjust_bk_index--;
		
	if(bk > wk3)
		adjust_bk_index--;
		
		
	return (_3K0C_AGAINST_1k1c_index(bc, adjust_wk1_index, adjust_wk2_index, adjust_wk3_index, adjust_bk_index) - 1);

}

unsigned long get_05_piece_index_3K0C_AGAINST_0k2c(unsigned char wk1, unsigned char wk2, unsigned char wk3, unsigned char bc1, unsigned char bc2)
{
	unsigned char 	adjust_wk1_index,
					adjust_wk2_index,
					adjust_wk3_index;
						
	adjust_wk1_index = wk1;
	adjust_wk2_index = wk2;
	adjust_wk3_index = wk3;
	
	if(wk1 > bc1)
		adjust_wk1_index--;
		
	if(wk1 > bc2)
		adjust_wk1_index--;

	if(wk2 > bc1)
		adjust_wk2_index--;
		
	if(wk2 > bc2)
		adjust_wk2_index--;

	if(wk3 > bc1)
		adjust_wk3_index--;
		
	if(wk3 > bc2)
		adjust_wk3_index--;
		
		
	return (_3K0C_AGAINST_0k2c_index(bc1, bc2, adjust_wk1_index, adjust_wk2_index, adjust_wk3_index) - 1);

}

unsigned long get_05_piece_index_2K1C_AGAINST_2k0c(unsigned char wk1, unsigned char wk2, unsigned char wc, unsigned char bk1, unsigned char bk2)
{
	unsigned char 	adjust_wk1_index,
					adjust_wk2_index,
					adjust_bk1_index,
					adjust_bk2_index;
						
	adjust_wk1_index = wk1;
	adjust_wk2_index = wk2;
	adjust_bk1_index = bk1;
	adjust_bk2_index = bk2;
	
	if(wk1 > wc)
		adjust_wk1_index--;
		
	if(wk2 > wc)
		adjust_wk2_index--;

	if(bk1 > wc)
		adjust_bk1_index--;
		
	if(bk2 > wc)
		adjust_bk2_index--;
		
	if(bk1 > wk1)
		adjust_bk1_index--;
		
	if(bk2 > wk1)
		adjust_bk2_index--;
		
	if(bk1 > wk2)
		adjust_bk1_index--;
		
	if(bk2 > wk2)
		adjust_bk2_index--;
		
		
	return (_2K1C_AGAINST_2k0c(wc-4, adjust_wk1_index, adjust_wk2_index, adjust_bk1_index, adjust_bk2_index) - 1);

}

unsigned long get_05_piece_index_2K1C_AGAINST_1k1c(unsigned char wk1, unsigned char wk2, unsigned char wc, unsigned char bk, unsigned char bc)
{
	unsigned char 	adjust_wk1_index,
					adjust_wk2_index,
					adjust_bk_index;

	unsigned long 	combined_checker_contribution_to_index;
	
	combined_checker_contribution_to_index = 1 + get_02_piece_index_0K1C_AGAINST_0k1c(wc, bc);						

	adjust_wk1_index = wk1;
	adjust_wk2_index = wk2;
	adjust_bk_index = bk;
	
	if(wk2 > bk)
		adjust_wk2_index--;
	
	if(wk2 > wc)
		adjust_wk2_index--;
		
	if(wk2 > bc)
		adjust_wk2_index--;
		
	if(wk1 > bk)
		adjust_wk1_index--;
	
	if(wk1 > wc)
		adjust_wk1_index--;
		
	if(wk1 > bc)
		adjust_wk1_index--;
		
	if(bk > wc)
		adjust_bk_index--;
		
	if(bk > bc)
		adjust_bk_index--;
		
		
	return (_2K1C_AGAINST_1k1c(combined_checker_contribution_to_index, adjust_bk_index, adjust_wk1_index, adjust_wk2_index) - 1);

}

unsigned long get_05_piece_index_2K1C_AGAINST_0k2c(unsigned char wk1, unsigned char wk2, unsigned char wc, unsigned char bc1, unsigned char bc2)
{
	unsigned char 	adjust_wc_index,
					adjust_bc1_index,
					adjust_bc2_index,
					adjust_wk1_index,
					adjust_wk2_index;

	unsigned long 	combined_checker_contribution_to_index;
							
	adjust_wc_index = wc;
	adjust_bc1_index = bc1;
	adjust_bc2_index = bc2;
	adjust_wk1_index = wk1;
	adjust_wk2_index = wk2;
	
	if(wk2 > bc2)
		adjust_wk2_index--;
		
	if(wk2 > bc1)
		adjust_wk2_index--;

	if(wk2 > wc)
		adjust_wk2_index--;



	if(wk1 > bc2)
		adjust_wk1_index--;
		
	if(wk1 > bc1)
		adjust_wk1_index--;

	if(wk1 > wc)
		adjust_wk1_index--;
		
		
		
	if(bc2 > wc)
		adjust_bc2_index--;
		
	if(bc1 > wc)
		adjust_bc1_index--;
	
	combined_checker_contribution_to_index = 0;
	
	if(wc > 28)
		{
			
			combined_checker_contribution_to_index = (378 * (wc - 29)) + _2_same_pieces_subindex(adjust_bc1_index, adjust_bc2_index);
		}
	else
		{
			
			combined_checker_contribution_to_index = (1512) + ((28 - wc) * 351) + _2_same_pieces_subindex(adjust_bc1_index, adjust_bc2_index);

		}
		
	return (_2K1C_AGAINST_0k2c(combined_checker_contribution_to_index, adjust_wk1_index, adjust_wk2_index) - 1);

}

unsigned long get_05_piece_index_1K2C_AGAINST_2k0c(unsigned char wk, unsigned char wc1, unsigned char wc2, unsigned char bk1, unsigned char bk2)
{
	unsigned char 	adjust_wk_index,
					adjust_wc1_index,
					adjust_wc2_index,
					adjust_bk1_index,
					adjust_bk2_index;
							
	adjust_wk_index = wk;
	adjust_wc1_index = wc1;
	adjust_wc2_index = wc2;
	adjust_bk1_index = bk1;
	adjust_bk2_index = bk2;
	
	adjust_wc1_index -= 4;
	adjust_wc2_index -= 4;
	
	if(wk > bk2)
		adjust_wk_index--;
		
	if(wk > bk1)
		adjust_wk_index--;
		
	if(wk > wc2)
		adjust_wk_index--;
		
	if(wk > wc1)
		adjust_wk_index--;
		
		
	if(bk2 > wc2)
		adjust_bk2_index--;
		
	if(bk2 > wc1)
		adjust_bk2_index--;
	
	if(bk1 > wc2)
		adjust_bk1_index--;
		
	if(bk1 > wc1)
		adjust_bk1_index--;
			
	return (_1K2C_AGAINST_2k0c(adjust_wc1_index, adjust_wc2_index, adjust_bk1_index, adjust_bk2_index, adjust_wk_index) - 1);
}

unsigned long get_05_piece_index_1K2C_AGAINST_1k1c(unsigned char wk, unsigned char wc1, unsigned char wc2, unsigned char bk, unsigned char bc)
{
	unsigned char 	adjust_bk_index,
					adjust_wk_index;

	unsigned long 	combined_checker_contribution_to_index;
	
	// order of placement: wc1, wc2, bc, wk, bk		
					
	adjust_bk_index = bk;
	adjust_wk_index = wk;
	
	combined_checker_contribution_to_index = 1 + get_03_piece_index_0K2C_AGAINST_0k1c(wc1, wc2, bc);
	
	if(wk > wc1)
		adjust_wk_index--;
		
	if(wk > wc2)
		adjust_wk_index--;
		
	if(wk > bc)
		adjust_wk_index--;

	////
	
	if(bk > wc1)
		adjust_bk_index--;
		
	if(bk > wc2)
		adjust_bk_index--;
		
	if(bk > bc)
		adjust_bk_index--;
		
	////
	
	if(bk > wk)
		adjust_bk_index--;				
		
	return (_1K2C_AGAINST_1k1c(combined_checker_contribution_to_index, adjust_wk_index, adjust_bk_index) - 1);

}

unsigned long get_05_piece_index_1K2C_AGAINST_0k2c(unsigned char wk, unsigned char wc1, unsigned char wc2, unsigned char bc1, unsigned char bc2)
{
	unsigned char 	adjust_wk_index;

	unsigned long 	combined_checker_contribution_to_index;
	
	combined_checker_contribution_to_index = 1 + get_04_piece_index_0K2C_AGAINST_0k2c(wc1, wc2, bc1, bc2);
						
	adjust_wk_index = wk;
			
	if(wk > bc1)
		adjust_wk_index--;
		
	if(wk > bc2)
		adjust_wk_index--;
		
	if(wk > wc1)
		adjust_wk_index--;
		
	if(wk > wc2)
		adjust_wk_index--;
				
		
	return (_1K2C_AGAINST_0k2c(combined_checker_contribution_to_index, adjust_wk_index) - 1);

}

unsigned long get_05_piece_index_0K3C_AGAINST_2k0c(unsigned char wc1, unsigned char wc2, unsigned char wc3, unsigned char bk1, unsigned char bk2)
{
	unsigned char 	adjust_bk1_index,
					adjust_bk2_index,
					adjust_wc1_index,
					adjust_wc2_index,
					adjust_wc3_index;
					
	adjust_bk1_index = bk1;
	adjust_bk2_index = bk2;
	
	adjust_wc1_index = wc1-4;
	adjust_wc2_index = wc2-4;
	adjust_wc3_index = wc3-4;
	
	if(bk1 > wc1)
		adjust_bk1_index--;
		
	if(bk1 > wc2)
		adjust_bk1_index--;
		
	if(bk1 > wc3)
		adjust_bk1_index--;
		
	if(bk2 > wc1)
		adjust_bk2_index--;
		
	if(bk2 > wc2)
		adjust_bk2_index--;
		
	if(bk2 > wc3)
		adjust_bk2_index--;
		
	return (_0K3C_AGAINST_2k0c_index(adjust_wc1_index, adjust_wc2_index, adjust_wc3_index, adjust_bk1_index, adjust_bk2_index) - 1);

}

unsigned long get_05_piece_index_0K3C_AGAINST_1k1c(unsigned char wc1, unsigned char wc2, unsigned char wc3, unsigned char bk, unsigned char bc)
{
		
	unsigned char 	adjust_bk_index;
	unsigned long 	combined_checker_contribution_to_index;
							
	combined_checker_contribution_to_index = 1 + get_04_piece_index_0K3C_AGAINST_0k1c(wc1, wc2, wc3, bc);
	
	adjust_bk_index = bk;
	
	if(bk > bc)
		adjust_bk_index--;
		
	if(bk > wc1)
		adjust_bk_index--;
		
	if(bk > wc2)
		adjust_bk_index--;
		
	if(bk > wc3)
		adjust_bk_index--;
		
		
	return (_0K3C_AGAINST_1k1c(combined_checker_contribution_to_index, adjust_bk_index) - 1);
	
}
unsigned long get_05_piece_index_0K3C_AGAINST_0k2c(unsigned char wc1, unsigned char wc2, unsigned char wc3, unsigned char bc1, unsigned char bc2)
{
	unsigned char 	adjust_bc1_index,
					adjust_bc2_index,
					adjust_wc1_index,
					adjust_wc2_index,
					adjust_wc3_index;

	unsigned long 	combined_checker_contribution_to_index;
							
	adjust_wc1_index = wc1 - 4;
	adjust_wc2_index = wc2 - 4;
	adjust_wc3_index = wc3 - 4;
	
	
	if((wc1 > bc1) && (bc1 >= 5))
		adjust_wc1_index--;
		
	if((wc1 > bc2) && (bc2 >= 5))
		adjust_wc1_index--;
		
	/*****/
		
	if((wc2 > bc1) && (bc1 >= 5))
		adjust_wc2_index--;
		
	if((wc2 > bc2) && (bc2 >= 5))
		adjust_wc2_index--;
		
	/*****/
		
	if((wc3 > bc1) && (bc1 >= 5))
		adjust_wc3_index--;
		
	if((wc3 > bc2) && (bc2 >= 5))
		adjust_wc3_index--;
	
	combined_checker_contribution_to_index = 0;
	
		
	if((bc1 < 5) && (bc2 < 5))
		{
			combined_checker_contribution_to_index = ((_2_same_pieces_subindex(bc1, bc2) - 1) * 3276) + (_3_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index, adjust_wc3_index));

		}
	else
	if((bc1 < 5) && (bc2 >= 5))
		{
			combined_checker_contribution_to_index = (19656) + ((((bc1 - 1) * 24) + (bc2-5)) * 2925) + (_3_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index, adjust_wc3_index));

		}
	else
		{
			adjust_bc1_index = bc1 - 4;
			adjust_bc2_index = bc2 - 4;
			
			combined_checker_contribution_to_index = (300456) + ((_2_same_pieces_subindex(adjust_bc1_index, adjust_bc2_index) - 1) * 2600) + (_3_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index, adjust_wc3_index));
		}
		
	return ((combined_checker_contribution_to_index) - 1);

}


unsigned long get_05_piece_index_4K0C_AGAINST_1k0c(unsigned char wk1, unsigned char wk2, unsigned char wk3, unsigned char wk4, unsigned char bk)
{
	unsigned char adjust_bk_index = bk;
	
	if(bk > wk1)
		adjust_bk_index--;
		
	if(bk > wk2)
		adjust_bk_index--;

	if(bk > wk3)
		adjust_bk_index--;

	if(bk > wk4)
		adjust_bk_index--;
			
	return (((_4_same_pieces_subindex(wk1, wk2, wk3, wk4) - 1) * 28) + adjust_bk_index - 1);
}

unsigned long  get_05_piece_index_4K0C_AGAINST_0k1c(unsigned char wk1, unsigned char wk2, unsigned char wk3, unsigned char wk4, unsigned char bc)
{
	unsigned char adjust_wk1_index = wk1,
	              adjust_wk2_index = wk2,
	              adjust_wk3_index = wk3,
	              adjust_wk4_index = wk4;
	
	
	if(wk1 > bc)
		adjust_wk1_index--;
		
	if(wk2 > bc)
		adjust_wk2_index--;

	if(wk3 > bc)
		adjust_wk3_index--;

	if(wk4 > bc)
		adjust_wk4_index--;
		
	
	return ((((bc - 1) * 31465) + _4_same_pieces_subindex(adjust_wk1_index, adjust_wk2_index, adjust_wk3_index, adjust_wk4_index)) - 1);
}


unsigned long  get_05_piece_index_3K1C_AGAINST_1k0c(unsigned char wk1, unsigned char wk2, unsigned char wk3, unsigned char wc1, unsigned char bk)
{
	unsigned char adjust_bk_index = bk;
	
	unsigned char adjust_wc1_index = wc1-4;
	
	unsigned char adjust_wk1_index = wk1;
	unsigned char adjust_wk2_index = wk2;
	unsigned char adjust_wk3_index = wk3;
	
	// order: wBWWW
	
	if(bk > wc1)
		adjust_bk_index--;
		
	if(wk1 > wc1)
		adjust_wk1_index--;
		
	if(wk2 > wc1)
		adjust_wk2_index--;

	if(wk3 > wc1)
		adjust_wk3_index--;
		
	if(wk1 > bk)
		adjust_wk1_index--;
		
	if(wk2 > bk)
		adjust_wk2_index--;

	if(wk3 > bk)
		adjust_wk3_index--;
	
	return (((adjust_wc1_index - 1) * 125860) + ((adjust_bk_index - 1) * 4060) + (_3_same_pieces_subindex(adjust_wk1_index, adjust_wk2_index, adjust_wk3_index)) - 1);
}

unsigned long  get_05_piece_index_3K1C_AGAINST_0k1c(unsigned char wk1, unsigned char wk2, unsigned char wk3, unsigned char wc1, unsigned char bc)
{
	//order: bwWWW 
	
	unsigned char adjust_wc1_index = wc1-4;
	
	unsigned char adjust_wk1_index = wk1;
	unsigned char adjust_wk2_index = wk2;
	unsigned char adjust_wk3_index = wk3;
	
	unsigned long combined_checker_contribution_to_index = 0;
	

	// can't just check if wc > bc, since wc starts out on square 5
	//
	if((bc > 4) && (wc1 > bc))
		adjust_wc1_index--;
	
	
	
	if(wk1 > wc1)
		adjust_wk1_index--;
		
	if(wk2 > wc1)
		adjust_wk2_index--;

	if(wk3 > wc1)
		adjust_wk3_index--;
		
	if(wk1 > bc)
		adjust_wk1_index--;
		
	if(wk2 > bc)
		adjust_wk2_index--;

	if(wk3 > bc)
		adjust_wk3_index--;
	
	if(bc < 5)
		{
			combined_checker_contribution_to_index = (28 * (bc - 1)) + adjust_wc1_index;
		}
	else
		{			
			combined_checker_contribution_to_index = (112) + ((bc - 5) * 27) + adjust_wc1_index;
		}
		
	return (((combined_checker_contribution_to_index - 1) * 4060) + (_3_same_pieces_subindex(adjust_wk1_index, adjust_wk2_index, adjust_wk3_index)) - 1);
}

unsigned long  get_05_piece_index_2K2C_AGAINST_1k0c(unsigned char wk1, unsigned char wk2, unsigned char wc1, unsigned char wc2, unsigned char bk)
{

	unsigned char adjust_bk_index = bk;
	
	unsigned char adjust_wc1_index = wc1-4;
	unsigned char adjust_wc2_index = wc2-4;

	unsigned char adjust_wk1_index = wk1;
	unsigned char adjust_wk2_index = wk2;
	
	// order: wwBWWW
	
	if(bk > wc1)
		adjust_bk_index--;
		
	if(wk1 > wc1)
		adjust_wk1_index--;
		
	if(wk2 > wc1)
		adjust_wk2_index--;
		
	if(bk > wc2)
		adjust_bk_index--;
		
	if(wk1 > wc2)
		adjust_wk1_index--;
		
	if(wk2 > wc2)
		adjust_wk2_index--;

	if(wk1 > bk)
		adjust_wk1_index--;
		
	if(wk2 > bk)
		adjust_wk2_index--;

	
	return (((_2_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index) - 1) * 12180) + ((adjust_bk_index - 1) * 406) +  _2_same_pieces_subindex(adjust_wk1_index, adjust_wk2_index) - 1);
}

unsigned long  get_05_piece_index_2K2C_AGAINST_0k1c(unsigned char wk1, unsigned char wk2, unsigned char wc1, unsigned char wc2, unsigned char bc)
{
	//order: bwwWW 
	
	unsigned char adjust_wc1_index = wc1-4;
	unsigned char adjust_wc2_index = wc2-4;
	
	unsigned char adjust_wk1_index = wk1;
	unsigned char adjust_wk2_index = wk2;
	
	unsigned long combined_checker_contribution_to_index = 0;
	

	// can't just check if wc > bc, since wc starts out on square 5
	//
	if((bc > 4) && (wc1 > bc))
		adjust_wc1_index--;
	
	if((bc > 4) && (wc2 > bc))
		adjust_wc2_index--;
	
	if(wk1 > wc1)
		adjust_wk1_index--;
		
	if(wk2 > wc1)
		adjust_wk2_index--;

	if(wk1 > wc2)
		adjust_wk1_index--;
		
	if(wk2 > wc2)
		adjust_wk2_index--;

	if(wk1 > bc)
		adjust_wk1_index--;
		
	if(wk2 > bc)
		adjust_wk2_index--;

	
	if(bc < 5)
		{
			combined_checker_contribution_to_index = (378 * (bc - 1)) + _2_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index);
		}
	else
		{			
			combined_checker_contribution_to_index = (1512) + ((bc - 5) * 351) + _2_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index);
		}
		
	return (((combined_checker_contribution_to_index - 1) * 406) + (_2_same_pieces_subindex(adjust_wk1_index, adjust_wk2_index)) - 1);
}

unsigned long  get_05_piece_index_1K3C_AGAINST_1k0c(unsigned char wk1, unsigned char wc1, unsigned char wc2, unsigned char wc3, unsigned char bk)
{
	unsigned char adjust_bk_index = bk;
	
	unsigned char adjust_wc1_index = wc1-4;
	unsigned char adjust_wc2_index = wc2-4;
	unsigned char adjust_wc3_index = wc3-4;

	unsigned char adjust_wk1_index = wk1;
	
	// order: wwwBW
	
	if(bk > wc1)
		adjust_bk_index--;
		
	if(bk > wc2)
		adjust_bk_index--;
		
	if(bk > wc3)
		adjust_bk_index--;
		
	if(wk1 > wc1)
		adjust_wk1_index--;
		
	if(wk1 > wc2)
		adjust_wk1_index--;
		
	if(wk1 > wc3)
		adjust_wk1_index--;
		
	if(wk1 > bk)
		adjust_wk1_index--;
		
	return (((_3_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index, adjust_wc3_index) - 1) * 812) + ((adjust_bk_index - 1) * 28) +  (adjust_wk1_index) - 1);
}

unsigned long  get_05_piece_index_1K3C_AGAINST_0k1c(unsigned char wk1, unsigned char wc1, unsigned char wc2, unsigned char wc3, unsigned char bc)
{
	//order: bwwwW 
	
	unsigned char adjust_wc1_index = wc1-4;
	unsigned char adjust_wc2_index = wc2-4;
	unsigned char adjust_wc3_index = wc3-4;

	unsigned char adjust_wk1_index = wk1;
	
	unsigned long combined_checker_contribution_to_index = 0;
	

	// can't just check if wc > bc, since wc starts out on square 5
	//
	if((bc > 4) && (wc1 > bc))
		adjust_wc1_index--;
	
	if((bc > 4) && (wc2 > bc))
		adjust_wc2_index--;
		
	if((bc > 4) && (wc3 > bc))
		adjust_wc3_index--;
	
	if(wk1 > wc1)
		adjust_wk1_index--;
		
	if(wk1 > wc2)
		adjust_wk1_index--;
		
	if(wk1 > wc3)
		adjust_wk1_index--;
		
	if(wk1 > bc)
		adjust_wk1_index--;
		
	
	if(bc < 5)
		{
			combined_checker_contribution_to_index = (3276 * (bc - 1)) + _3_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index, adjust_wc3_index);
		}
	else
		{			
			combined_checker_contribution_to_index = (13104) + ((bc - 5) * 2925) + _3_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index, adjust_wc3_index);
		}
		
	return (((combined_checker_contribution_to_index - 1) * 28) + (adjust_wk1_index) - 1);
}

unsigned long  get_05_piece_index_0K4C_AGAINST_1k0c(unsigned char wc1, unsigned char wc2, unsigned char wc3, unsigned char wc4, unsigned char bk)
{
	unsigned char adjust_bk_index = bk;
	
	unsigned char adjust_wc1_index = wc1-4;
	unsigned char adjust_wc2_index = wc2-4;
	unsigned char adjust_wc3_index = wc3-4;
	unsigned char adjust_wc4_index = wc4-4;
	
	// order: wwwwB
	
	if(bk > wc1)
		adjust_bk_index--;
		
	if(bk > wc2)
		adjust_bk_index--;
		
	if(bk > wc3)
		adjust_bk_index--;
		
	if(bk > wc4)
		adjust_bk_index--;
		
		
	return (((_4_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index, adjust_wc3_index, adjust_wc4_index) - 1) * 28) + (adjust_bk_index) - 1);
}

unsigned long  get_05_piece_index_0K4C_AGAINST_0k1c(unsigned char wc1, unsigned char wc2, unsigned char wc3, unsigned char wc4, unsigned char bc)
{
	//order: bwwww 
		
	unsigned char adjust_wc1_index = wc1-4;
	unsigned char adjust_wc2_index = wc2-4;
	unsigned char adjust_wc3_index = wc3-4;
	unsigned char adjust_wc4_index = wc4-4;
	
	unsigned long combined_checker_contribution_to_index = 0;

	// can't just check if wc > bc, since wc starts out on square 5
	//
	if((bc > 4) && (wc1 > bc))
		adjust_wc1_index--;
	
	if((bc > 4) && (wc2 > bc))
		adjust_wc2_index--;
		
	if((bc > 4) && (wc3 > bc))
		adjust_wc3_index--;
		
	if((bc > 4) && (wc4 > bc))
		adjust_wc4_index--;
			
	
	if(bc < 5)
		{
			combined_checker_contribution_to_index = (20475 * (bc - 1)) + _4_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index, adjust_wc3_index, adjust_wc4_index);
		}
	else
		{			
			combined_checker_contribution_to_index = (81900) + ((bc - 5) * 17550) + _4_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index, adjust_wc3_index, adjust_wc4_index);
		}
		
	return (combined_checker_contribution_to_index - 1);
}

unsigned long get_05_piece_index_for_slice(unsigned char which_slice, unsigned char square1, unsigned char square2, unsigned char square3, unsigned char square4, unsigned char square5)
{
	unsigned long index_function_value;
	
	index_function_value = 0;
	
	switch(which_slice)
	{
		  case k_WWWRR:
		  	   index_function_value = get_05_piece_index_3K0C_AGAINST_2k0c(square1, square2, square3, square4, square5);
		  break;
		  case k_RRRWW:
		  	   index_function_value = get_05_piece_index_3K0C_AGAINST_2k0c(33-square3, 33-square2, 33-square1, 33-square5, 33-square4);
		  break;
		  case k_WWWRr:
		  	   index_function_value = get_05_piece_index_3K0C_AGAINST_1k1c(square1, square2, square3, square4, square5);
		  break;
		  case k_RRRWw:
		  	   index_function_value = get_05_piece_index_3K0C_AGAINST_1k1c(33-square3, 33-square2, 33-square1, 33-square4, 33-square5);
		  break;
		  case k_WWWrr:
		  	   index_function_value = get_05_piece_index_3K0C_AGAINST_0k2c(square1, square2, square3,square4, square5);
		  break;
		  case k_RRRww:
		  	   index_function_value = get_05_piece_index_3K0C_AGAINST_0k2c(33-square3, 33-square2, 33-square1, 33-square5, 33-square4);
		  break;                                  
		  case k_WWwRR:
		  	   index_function_value = get_05_piece_index_2K1C_AGAINST_2k0c(square1, square2, square3, square4, square5);
		  break;
		  case k_RRrWW:
		  	   index_function_value = get_05_piece_index_2K1C_AGAINST_2k0c(33-square2, 33-square1, 33-square3, 33-square5, 33-square4);
		  break;
		  case k_WWwRr:
		  	   index_function_value = get_05_piece_index_2K1C_AGAINST_1k1c(square1, square2, square3, square4, square5);
		  break;

		  case k_RRrWw:
		  	   index_function_value = get_05_piece_index_2K1C_AGAINST_1k1c(33-square2, 33-square1, 33-square3, 33-square4, 33-square5);
		  break;
		  case k_WWwrr:
		  	   index_function_value = get_05_piece_index_2K1C_AGAINST_0k2c(square1, square2, square3, square4, square5);
		  break;
		  case k_RRrww:
		  	   index_function_value = get_05_piece_index_2K1C_AGAINST_0k2c(33-square2, 33-square1, 33-square3, 33-square5, 33-square4);
		  break;
		  case k_WwwRR:
		  	   index_function_value = get_05_piece_index_1K2C_AGAINST_2k0c(square1, square2, square3, square4, square5);
		  break;
		  case k_RrrWW:
		  	   index_function_value = get_05_piece_index_1K2C_AGAINST_2k0c(33-square1, 33-square3, 33-square2, 33-square5, 33-square4);
		  break;
		  case k_WwwRr:
		  	   index_function_value = get_05_piece_index_1K2C_AGAINST_1k1c(square1, square2, square3, square4, square5);
		  break;

		  case k_RrrWw:
		  	   index_function_value = get_05_piece_index_1K2C_AGAINST_1k1c(33-square1, 33-square3, 33-square2, 33-square4, 33-square5);
		  break;
		  case k_Wwwrr:
		       index_function_value = get_05_piece_index_1K2C_AGAINST_0k2c(square1, square2, square3, square4, square5);
		  break;
		  case k_Rrrww:
		       index_function_value = get_05_piece_index_1K2C_AGAINST_0k2c(33-square1, 33-square3, 33-square2, 33-square5, 33-square4);
		  break;
		  case k_wwwRR:
		  	   index_function_value = get_05_piece_index_0K3C_AGAINST_2k0c(square1, square2, square3, square4, square5);
		  break;
		  case k_rrrWW:
		  	   index_function_value = get_05_piece_index_0K3C_AGAINST_2k0c(33-square3, 33-square2, 33-square1, 33-square5, 33-square4);
		  break;
		  case k_wwwRr:
		       index_function_value = get_05_piece_index_0K3C_AGAINST_1k1c(square1, square2, square3, square4, square5);
		  break;
		  case k_rrrWw:
		       index_function_value = get_05_piece_index_0K3C_AGAINST_1k1c(33-square3, 33-square2, 33-square1, 33-square4, 33-square5);
		  break;
		  case k_wwwrr:
		  	   index_function_value = get_05_piece_index_0K3C_AGAINST_0k2c(square1, square2, square3, square4, square5);
		  break;
		  case k_rrrww:
		  	   index_function_value = get_05_piece_index_0K3C_AGAINST_0k2c(33-square3, 33-square2, 33-square1, 33-square5, 33-square4);
		  break;
	}
	return index_function_value;
}



unsigned long get_06_piece_index_3K0C_AGAINST_3k0c(unsigned char wk1, unsigned char wk2, unsigned char wk3, unsigned char bk1, unsigned char bk2, unsigned char bk3)
{
	unsigned long index_function_value;
	unsigned char 	adjust_bk1_index,
					adjust_bk2_index,
					adjust_bk3_index;

	index_function_value = 0;
	
					
	adjust_bk1_index = bk1;
	adjust_bk2_index = bk2;
	adjust_bk3_index = bk3;
	
	if(bk1 > wk1)
		adjust_bk1_index--;
		
	if(bk1 > wk2)
		adjust_bk1_index--;
		
	if(bk1 > wk3)
		adjust_bk1_index--;
		
	if(bk2 > wk1)
		adjust_bk2_index--;
		
	if(bk2 > wk2)
		adjust_bk2_index--;
		
	if(bk2 > wk3)
		adjust_bk2_index--;
		
	if(bk3 > wk1)
		adjust_bk3_index--;
		
	if(bk3 > wk2)
		adjust_bk3_index--;
		
	if(bk3 > wk3)
		adjust_bk3_index--;
		
	
	index_function_value = (_3K0C_AGAINST_3k0c_index(wk1, wk2, wk3, adjust_bk1_index, adjust_bk2_index, adjust_bk3_index) - 1);
	
	return index_function_value;
}

unsigned long get_06_piece_index_3K0C_AGAINST_2k1c(unsigned char wk1, unsigned char wk2, unsigned char wk3, unsigned char bk1, unsigned char bk2, unsigned char bc1)
{
	unsigned long index_function_value;
	unsigned char 	adjust_bk1_index,
					adjust_bk2_index,
					adjust_wk1_index,
					adjust_wk2_index,
					adjust_wk3_index;

	index_function_value = 0;
	
	// order: rWWWRR
	
	adjust_wk1_index = wk1;
	adjust_wk2_index = wk2;
	adjust_wk3_index = wk3;
					
	adjust_bk1_index = bk1;
	adjust_bk2_index = bk2;

	if(wk1 > bc1)
		adjust_wk1_index--;
	if(wk2 > bc1)
		adjust_wk2_index--;
	if(wk3 > bc1)
		adjust_wk3_index--;

	if(bk1 > bc1)
		adjust_bk1_index--;
	if(bk2 > bc1)
		adjust_bk2_index--;
		
	if(bk1 > wk1)
		adjust_bk1_index--;
	if(bk1 > wk2)
		adjust_bk1_index--;
	if(bk1 > wk3)
		adjust_bk1_index--;

	if(bk2 > wk1)
		adjust_bk2_index--;
	if(bk2 > wk2)
		adjust_bk2_index--;
	if(bk2 > wk3)
		adjust_bk2_index--;

	index_function_value = (_3K0C_AGAINST_2k1c_index(bc1, adjust_wk1_index, adjust_wk2_index, adjust_wk3_index, adjust_bk1_index, adjust_bk2_index) - 1);
	
	return index_function_value;
}



unsigned long get_06_piece_index_3K0C_AGAINST_1k2c(unsigned char wk1, unsigned char wk2, unsigned char wk3, unsigned char bk1, unsigned char bc1, unsigned char bc2)
{
	unsigned long index_function_value;
	unsigned char 	adjust_bk1_index,
					adjust_wk1_index,
					adjust_wk2_index,
					adjust_wk3_index;

	index_function_value = 0;
	
	// order: rrWWWB
	
	adjust_wk1_index = wk1;
	adjust_wk2_index = wk2;
	adjust_wk3_index = wk3;
					
	adjust_bk1_index = bk1;

	if(wk1 > bc1)
		adjust_wk1_index--;
	if(wk2 > bc1)
		adjust_wk2_index--;
	if(wk3 > bc1)
		adjust_wk3_index--;


	if(wk1 > bc2)
		adjust_wk1_index--;
	if(wk2 > bc2)
		adjust_wk2_index--;
	if(wk3 > bc2)
		adjust_wk3_index--;


	if(bk1 > bc1)
		adjust_bk1_index--;
	if(bk1 > bc2)
		adjust_bk1_index--;
		
	if(bk1 > wk1)
		adjust_bk1_index--;
	if(bk1 > wk2)
		adjust_bk1_index--;
	if(bk1 > wk3)
		adjust_bk1_index--;

	index_function_value = (_3K0C_AGAINST_1k2c_index(bc1, bc2, adjust_wk1_index, adjust_wk2_index, adjust_wk3_index, adjust_bk1_index) - 1);
	
	return index_function_value;
}

unsigned long get_06_piece_index_3K0C_AGAINST_0k3c(unsigned char wk1, unsigned char wk2, unsigned char wk3, unsigned char bc1, unsigned char bc2, unsigned char bc3)
{
	unsigned long index_function_value;
	unsigned char 	adjust_wk1_index,
					adjust_wk2_index,
					adjust_wk3_index;

	index_function_value = 0;
					
	adjust_wk1_index = wk1;
	adjust_wk2_index = wk2;
	adjust_wk3_index = wk3;
	
	if(wk1 > bc1)
		adjust_wk1_index--;
		
	if(wk1 > bc2)
		adjust_wk1_index--;
		
	if(wk1 > bc3)
		adjust_wk1_index--;
		
	if(wk2 > bc1)
		adjust_wk2_index--;
		
	if(wk2 > bc2)
		adjust_wk2_index--;
		
	if(wk2 > bc3)
		adjust_wk2_index--;
		
	if(wk3 > bc1)
		adjust_wk3_index--;
		
	if(wk3 > bc2)
		adjust_wk3_index--;
		
	if(wk3 > bc3)
		adjust_wk3_index--;
		
	
	index_function_value = (_3K0C_AGAINST_0k3c_index(bc1, bc2, bc3, adjust_wk1_index, adjust_wk2_index, adjust_wk3_index) - 1);
	
	return index_function_value;
}



unsigned long get_06_piece_index_2K1C_AGAINST_2k1c(unsigned char wk1, unsigned char wk2, unsigned char wc1, unsigned char bk1, unsigned char bk2,  unsigned char bc1)
{
	unsigned long index_function_value, combined_checker_contribution_to_index;
	unsigned char 	adjust_bk1_index,
					adjust_bk2_index,
					adjust_wk1_index,
					adjust_wk2_index;

	// order: bwRRWW
	
	index_function_value = 0;
	combined_checker_contribution_to_index = 0;
	
		
	adjust_bk1_index = bk1;
	adjust_bk2_index = bk2;

	adjust_wk1_index = wk1;
	adjust_wk2_index = wk2;

		
	/************/
	
	if(wk2 > bk2)
		adjust_wk2_index--;

	if(wk2 > bk1)
		adjust_wk2_index--;

	if(wk2 > wc1)
		adjust_wk2_index--;

	if(wk2 > bc1)
		adjust_wk2_index--;
		
	/************/
	
	if(wk1 > bk2)
		adjust_wk1_index--;

	if(wk1 > bk1)
		adjust_wk1_index--;

	if(wk1 > wc1)
		adjust_wk1_index--;

	if(wk1 > bc1)
		adjust_wk1_index--;
		
	/************/
	
	if(bk2 > wc1)
		adjust_bk2_index--;

	if(bk2 > bc1)
		adjust_bk2_index--;
		
	/************/
	
	if(bk1 > wc1)
		adjust_bk1_index--;

	if(bk1 > bc1)
		adjust_bk1_index--;
		
	/************/
			
		
	combined_checker_contribution_to_index = 0;
	
	if(bc1 < 5)
		{
			combined_checker_contribution_to_index = (28 * (bc1 - 1)) + (wc1 - 4);
		}
	else
		{
			if(wc1 > bc1)
				combined_checker_contribution_to_index = (112) + (bc1 - 5)*27 + (wc1 - 5);
			else
				combined_checker_contribution_to_index = (112) + (bc1 - 5)*27 + (wc1 - 4);
		}
		
	
	index_function_value = (_2K1C_AGAINST_2k1c_index(combined_checker_contribution_to_index, adjust_bk1_index, adjust_bk2_index, adjust_wk1_index, adjust_wk2_index) - 1);
	
	return index_function_value;
}

unsigned long get_06_piece_index_1K2C_AGAINST_2k1c(unsigned char wk1, unsigned char wc1, unsigned char wc2, unsigned char bk1, unsigned char bk2, unsigned char bc1)
{
	unsigned char 	adjust_wc1_index,
					adjust_wc2_index,
					adjust_bk1_index,
					adjust_bk2_index,
					adjust_wk1_index;

	unsigned long 	combined_checker_contribution_to_index;
	
	adjust_wk1_index = wk1;
	
	adjust_bk1_index = bk1;
	adjust_bk2_index = bk2;
	
	adjust_wc1_index = wc1;
	adjust_wc2_index = wc2;
	
	if(wk1 > bk2)
		adjust_wk1_index--;
		
	if(wk1 > bk1)
		adjust_wk1_index--;
		
	if(wk1 > wc2)
		adjust_wk1_index--;
		
	if(wk1 > wc1)
		adjust_wk1_index--;
		
	if(wk1 > bc1)
		adjust_wk1_index--;
		
	/*********************/
		
	if(bk1 > wc2)
		adjust_bk1_index--;
		
	if(bk1 > wc1)
		adjust_bk1_index--;
		
	if(bk1 > bc1)
		adjust_bk1_index--;
		
	/*********************/
		
	if(bk2 > wc2)
		adjust_bk2_index--;
		
	if(bk2 > wc1)
		adjust_bk2_index--;
		
	if(bk2 > bc1)
		adjust_bk2_index--;

	/*************************************************************************************************/
	/*                                                                                               */
	/* Ed Trice July 11, 2001                                                                        */
	/*                                                                                               */
	/* With one black checker in the king row, count from 1-378 for each of the 4 squares.           */
	/* The white checkers can reside on 28*27/2  = 378 squares, so the equations is of the form:     */
	/*                                                                                               */
	/* (bc-1)* 378 + _2_same_pieces_subindex(wc1, wc2)                                               */
	/*                                                                                               */
	/* When the black checker has moved to square 5 or greater, then you add 378*4 (1512) to the     */
	/* count and determine which of the 27*26/2 remaining combination of squares the white checkers  */
	/* are on. With 27*26/2 = 351, the equation looks like...                                        */
	/*                                                                                               */
	/* (bc-5)* 351 + _2_same_pieces_subindex(wc1, wc2) + 1512                                        */
	/*                                                                                               */
	/* This technique treats the black and white checkers as one "sub-index", rather than separate   */
	/* components. We count from 1 to (4*378 + 24*351) = 9936, then pass this as the "combined" to   */
	/* the  precompiled macro. This "Q" is a substitue for "bc" and "wc1" and "wc2".                 */
	/*                                                                                               */
	/*************************************************************************************************/
	
	combined_checker_contribution_to_index = 0;
	
	if(bc1 < 5)
		{
			adjust_wc1_index -= 4;
			adjust_wc2_index -= 4;
			
			combined_checker_contribution_to_index = (378 * (bc1 - 1)) + _2_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index);
		}
	else
		{
			if(wc1 > bc1) 
				adjust_wc1_index -= 5;
			else
				adjust_wc1_index -= 4;

			if(wc2 > bc1) 
				adjust_wc2_index -= 5;
			else
				adjust_wc2_index -= 4;

			
			combined_checker_contribution_to_index = (1512) + (bc1 - 5)*351 + _2_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index);

		}
		
		
	return (_1K2C_AGAINST_2k1c_index(combined_checker_contribution_to_index, adjust_bk1_index, adjust_bk2_index, adjust_wk1_index) - 1);

}

unsigned long get_06_piece_index_1K2C_AGAINST_1k2c(unsigned char wk1, unsigned char wc1, unsigned char wc2, unsigned char bk1, unsigned char bc1, unsigned char bc2)
{
	unsigned char 	adjust_wc1_index,
					adjust_wc2_index,
					adjust_bk1_index,
					adjust_wk1_index;

	unsigned long 	combined_checker_contribution_to_index;
	
	adjust_wk1_index = wk1;
	
	adjust_bk1_index = bk1;
	
	adjust_wc1_index = wc1;
	adjust_wc2_index = wc2;
						
	/**********************/
	
	if(wk1 > bc1)
		adjust_wk1_index--;
		
	if(wk1 > bc2)
		adjust_wk1_index--;
		
	if(wk1 > wc1)
		adjust_wk1_index--;
		
	if(wk1 > wc2)
		adjust_wk1_index--;
		
	if(wk1 > bk1)
		adjust_wk1_index--;
		
	/**********************/
	
	if(bk1 > bc1)
		adjust_bk1_index--;
		
	if(bk1 > bc2)
		adjust_bk1_index--;
		
	if(bk1 > wc1)
		adjust_bk1_index--;
		
	if(bk1 > wc2)
		adjust_bk1_index--;
		
	/**********************/
		
	adjust_wc1_index -= 4;
	adjust_wc2_index -= 4;
	
	/**********************/

	combined_checker_contribution_to_index = 0;
	
	if((bc1 < 5) && (bc2 < 5))
		{
			
			combined_checker_contribution_to_index = (378 * (_2_same_pieces_subindex(bc1, bc2) -1)) + _2_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index);
		}
	else
	if((bc1 < 5) && (bc2 > 4))

		{
		
			if(wc1 > bc2)
				adjust_wc1_index--;

			if(wc2 > bc2)
				adjust_wc2_index--;
			
			combined_checker_contribution_to_index = (2268) + (((bc1-1)*24 + (bc2 - 5)) * 351) + _2_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index);

		}
	else
		{
			if(wc1 > bc1)
				adjust_wc1_index--;

			if(wc1 > bc2)
				adjust_wc1_index--;
		
		
			if(wc2 > bc1)
				adjust_wc2_index--;

			if(wc2 > bc2)
				adjust_wc2_index--;
				
			combined_checker_contribution_to_index = (35964) + (325 * (_2_same_pieces_subindex(bc1-4, bc2-4) -1)) + _2_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index);
		}
		
	return (_1K2C_AGAINST_1k2c_index(combined_checker_contribution_to_index, adjust_bk1_index, adjust_wk1_index) - 1);
}

unsigned long get_06_piece_index_0K3C_AGAINST_2k1c(unsigned char wc1, unsigned char wc2, unsigned char wc3, unsigned char bk1, unsigned char bk2, unsigned char bc1)
{
	unsigned long index_function_value, combined_checker_contribution_to_index;
	unsigned char 	adjust_bk1_index,
					adjust_bk2_index,
					adjust_wc1_index,
					adjust_wc2_index,
					adjust_wc3_index;

	// order: bwwwRR
	index_function_value = 0;
	combined_checker_contribution_to_index = 0;
	
	adjust_bk1_index = bk1;
	adjust_bk2_index = bk2;
		
	adjust_wc1_index = wc1;
	adjust_wc2_index = wc2;
	adjust_wc3_index = wc3;
	
	/********************/
	
	if(bk1 > bc1)
		adjust_bk1_index--;
	
	if(bk1 > wc1)
		adjust_bk1_index--;
	
	if(bk1 > wc2)
		adjust_bk1_index--;
	
	if(bk1 > wc3)
		adjust_bk1_index--;
		
	/********************/
	
	
	if(bk2 > bc1)
		adjust_bk2_index--;
	
	if(bk2 > wc1)
		adjust_bk2_index--;
	
	if(bk2 > wc2)
		adjust_bk2_index--;
	
	if(bk2 > wc3)
		adjust_bk2_index--;

	/***************************************************************************************************/
	/*                                                                                                 */
	/* Ed Trice March 3, 2002 ===> Preparing for 6-piece perfect play, borrowing code from 7-piece idx */
	/*                                                                                                 */
	/* With one black checker in the king row, count from 1-3276 for each of the 4 squares.            */
	/* The white checkers can reside on 28*27*26/6  = 3276 squares, so the equations is of the form:   */
	/*                                                                                                 */
	/* (bc-1)* 3276 + _3_same_pieces_subindex(wc1, wc2, wc3)                                           */
	/*                                                                                                 */
	/* When the black checker has moved to square 5 or greater, then you add 3276*4 (13104) to the     */
	/* count and determine which of the 27*26*25/6 remaining combination of squares the white checkers */
	/* are on. With 27*26*25/6 = 2925, the equation looks like...                                      */
	/*                                                                                                 */
	/* (bc-5)* 2925 + _2_same_pieces_subindex(wc1, wc2, wc3) + 13104                                   */
	/*                                                                                                 */
	/* This technique treats the black and white checkers as one "sub-index", rather than separate     */
	/* components. We count from 1 to (4*3276 + 24*2925) = 83304, then pass this as the "combined" to  */
	/* the  precompiled macro. This "Q" is a substitue for "bc" and "wc1",  "wc2", and "wc3".          */
	/*                                                                                                 */
	/***************************************************************************************************/
	
	combined_checker_contribution_to_index = 0;
	
	if(bc1 < 5)
		{
			adjust_wc1_index -= 4;
			adjust_wc2_index -= 4;
			adjust_wc3_index -= 4;
			
			combined_checker_contribution_to_index = (3276 * (bc1 - 1)) + _3_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index, adjust_wc3_index);
		}
	else
		{
			if(wc1 > bc1) 
				adjust_wc1_index -= 5;
			else
				adjust_wc1_index -= 4;

			if(wc2 > bc1) 
				adjust_wc2_index -= 5;
			else
				adjust_wc2_index -= 4;
				
			if(wc3 > bc1) 
				adjust_wc3_index -= 5;
			else
				adjust_wc3_index -= 4;

			
			combined_checker_contribution_to_index = (13104) + (bc1 - 5)*2925 + _3_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index, adjust_wc3_index);

		}

	return (_0K3C_AGAINST_2k1c_index(combined_checker_contribution_to_index, adjust_bk1_index, adjust_bk2_index) -1);
}

unsigned long get_06_piece_index_0K3C_AGAINST_1k2c(unsigned char wc1, unsigned char wc2, unsigned char wc3, unsigned char bk1, unsigned char bc1, unsigned char bc2)
{
	unsigned long index_function_value, combined_checker_contribution_to_index;
	unsigned char 	adjust_bk1_index,
					adjust_wc1_index,
					adjust_wc2_index,
					adjust_wc3_index;

	// order: rrwwwB
	index_function_value = 0;
	combined_checker_contribution_to_index = 0;
	
	adjust_bk1_index = bk1;
		
	adjust_wc1_index = wc1-4;
	adjust_wc2_index = wc2-4;
	adjust_wc3_index = wc3-4;
	

	if(bk1 > bc1)
		adjust_bk1_index--;
	if(bk1 > bc2)
		adjust_bk1_index--;
		
	if(bk1 > wc1)
		adjust_bk1_index--;
	if(bk1 > wc2)
		adjust_bk1_index--;
	if(bk1 > wc3)
		adjust_bk1_index--;

	/********************/
	
	if(bc1 >= 5)
	{
		if(wc1 > bc1)
			adjust_wc1_index--;
			
		if(wc2 > bc1)
			adjust_wc2_index--;
			
		if(wc3 > bc1)
			adjust_wc3_index--;
	}		

	
	if(bc2 >= 5)
	{
		if(wc1 > bc2)
			adjust_wc1_index--;
			
		if(wc2 > bc2)
			adjust_wc2_index--;
			
		if(wc3 > bc2)
			adjust_wc3_index--;
	}
	
	combined_checker_contribution_to_index = 0;
	
	if((bc1 < 5) && (bc2 < 5))
		{
			
			combined_checker_contribution_to_index = (((_2_same_pieces_subindex(bc1, bc2)) -1) * 3276) + _3_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index, adjust_wc3_index);
		}
	else
	if((bc1 < 5) && (bc2 > 4))
		{
			combined_checker_contribution_to_index = (19656) + ((((bc1 - 1) * 24) + (bc2 - 5)) * 2925) + _3_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index, adjust_wc3_index);
		}
	else
	if((bc1 > 4) && (bc2 > 4))
		{
			combined_checker_contribution_to_index = (300456) + (((_2_same_pieces_subindex(bc1 - 4, bc2 - 4)) -1) * 2600) + _3_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index, adjust_wc3_index);
		}

	return (_0K3C_AGAINST_1k2c_index(combined_checker_contribution_to_index, adjust_bk1_index) -1);
}

unsigned long get_06_piece_index_0K3C_AGAINST_0k3c(unsigned char wc1, unsigned char wc2, unsigned char wc3, unsigned char bc1, unsigned char bc2, unsigned char bc3)
{
	unsigned long   combined_checker_contribution_to_index;
	unsigned char 	adjust_wc1_index,
					adjust_wc2_index,
					adjust_wc3_index;

	combined_checker_contribution_to_index = 0;
	
	adjust_wc1_index = wc1 - 4;
	adjust_wc2_index = wc2 - 4;
	adjust_wc3_index = wc3 - 4;
	
	/********************/
	
	if(bc1 >= 5)
	{
		if(wc1 > bc1)
			adjust_wc1_index--;
			
		if(wc2 > bc1)
			adjust_wc2_index--;
			
		if(wc3 > bc1)
			adjust_wc3_index--;
	}		

	
	if(bc2 >= 5)
	{
		if(wc1 > bc2)
			adjust_wc1_index--;
			
		if(wc2 > bc2)
			adjust_wc2_index--;
			
		if(wc3 > bc2)
			adjust_wc3_index--;
	}		
	
	if(bc3 >= 5)
	{
		if(wc1 > bc3)
			adjust_wc1_index--;
			
		if(wc2 > bc3)
			adjust_wc2_index--;
			
		if(wc3 > bc3)
			adjust_wc3_index--;
			
	}		
		
		
	if((bc1 < 5) && (bc2 < 5) && (bc3 < 5))
		{
			combined_checker_contribution_to_index = (((_3_same_pieces_subindex(bc1, bc2, bc3)) -1) * 3276) + _3_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index, adjust_wc3_index);
		}
	else
	if((bc1 < 5) && (bc2 < 5) && (bc3 > 4))
		{
			combined_checker_contribution_to_index = (13104) + ((((_2_same_pieces_subindex(bc1, bc2) - 1) * 24) + (bc3 - 5)) * 2925) + _3_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index, adjust_wc3_index);
		}
	else
	if((bc1 < 5) && (bc2 > 4) && (bc3 > 4))
		{
			combined_checker_contribution_to_index = (434304) + ((((bc1 - 1) * 276) + (_2_same_pieces_subindex(bc2 - 4, bc3 - 4) -1)) * 2600) + _3_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index, adjust_wc3_index);
		}
	else
	if((bc1 > 4) && (bc2 > 4) && (bc3 > 4))
		{
			combined_checker_contribution_to_index = (3304704) + ((_3_same_pieces_subindex(bc1-4, bc2-4, bc3-4) -1) * 2300) + _3_same_pieces_subindex(adjust_wc1_index, adjust_wc2_index, adjust_wc3_index);
		}

	return (combined_checker_contribution_to_index - 1);
	
}

/*******************************************************/
/*                                                     */
/* 6 piece Indexing Function Procedures                */
/*                                                     */
/*******************************************************/

unsigned long get_06_piece_index_for_slice(unsigned char which_slice, unsigned char square1, unsigned char square2, unsigned char square3, unsigned char square4, unsigned char square5, unsigned char square6)
{
	unsigned long index_function_value;
	
	index_function_value = 0;
	
	switch(which_slice)
	{
	
	 case k_WWWRRR:
	      index_function_value = get_06_piece_index_3K0C_AGAINST_3k0c(square1, square2, square3, square4, square5, square6);
	 break;
	 case k_WWwRRR:
	      index_function_value = get_06_piece_index_3K0C_AGAINST_2k1c(33-square6, 33-square5, 33-square4, 33-square2, 33-square1, 33-square3);
	 break;
	 case k_WwwRRR:
	      index_function_value = get_06_piece_index_3K0C_AGAINST_1k2c(33-square6, 33-square5, 33-square4, 33-square1, 33-square3, 33-square2);
	 break;
	 case k_wwwRRR:
	      index_function_value = get_06_piece_index_3K0C_AGAINST_0k3c(33-square6, 33-square5, 33-square4, 33-square3, 33-square2, 33-square1);
	 break;	 
	 
	 case k_WWWRRr:
	      index_function_value = get_06_piece_index_3K0C_AGAINST_2k1c(square1, square2, square3, square4, square5, square6);
	 break;
	 case k_WWwRRr:
	      index_function_value = get_06_piece_index_2K1C_AGAINST_2k1c(square1, square2, square3, square4, square5, square6);
	 break;
	 case k_WwwRRr:
	      index_function_value = get_06_piece_index_1K2C_AGAINST_2k1c(square1, square2, square3, square4, square5, square6);
	 break;
	 case k_wwwRRr:
	      index_function_value = get_06_piece_index_0K3C_AGAINST_2k1c(square1, square2, square3, square4, square5, square6);
	 break;	 
	 
	 case k_WWWRrr:
	      index_function_value = get_06_piece_index_3K0C_AGAINST_1k2c(square1, square2, square3, square4, square5, square6);
	 break;
	 case k_WWwRrr:
	      index_function_value = get_06_piece_index_1K2C_AGAINST_2k1c(33-square4, 33-square6, 33-square5, 33-square2, 33-square1, 33-square3);
	 break;
	 case k_WwwRrr:
	      index_function_value = get_06_piece_index_1K2C_AGAINST_1k2c(square1, square2, square3, square4, square5, square6);
	 break;
	 case k_wwwRrr:
	      index_function_value = get_06_piece_index_0K3C_AGAINST_1k2c(square1, square2, square3, square4, square5, square6);
	 break;
	 
	 case k_WWWrrr:
	      index_function_value = get_06_piece_index_3K0C_AGAINST_0k3c(square1, square2, square3, square4, square5, square6);
	 break;
	 case k_WWwrrr:
	      index_function_value = get_06_piece_index_0K3C_AGAINST_2k1c(33-square6, 33-square5, 33-square4, 33-square2, 33-square1, 33-square3);
	 break;
	 case k_Wwwrrr:
	      index_function_value = get_06_piece_index_0K3C_AGAINST_1k2c(33-square6, 33-square5, 33-square4, 33-square1, 33-square3, 33-square2);
	 break;
	 case k_wwwrrr:
	      index_function_value = get_06_piece_index_0K3C_AGAINST_0k3c(square1, square2, square3, square4, square5, square6);
	 break;
	}

	return index_function_value;
}


/************************************************************************************************/
/* Databases																					*/
/* 2-piece to 6-piece																			*/
/*																								*/
/* The database array is indexed by the slice indices (e.g. k_WR)								*/
/* note : array size is 1 bigger than actual size to be since the slice indices begin at 1		*/
/* 6-piece database contains 3 against 3 only													*/
/************************************************************************************************/
unsigned char* g_db2[ 5 ]  = { NULL, NULL, NULL, NULL, NULL };
unsigned char* g_db3[ 13 ] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
unsigned char* g_db4[ 26 ] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
unsigned char* g_db5[ 25 ] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
unsigned char* g_db6[ 17 ] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };


void close_trice_egdb(SDatabaseInfo &dbInfo)
{
	struct Dblist {
		unsigned char **slices;
		size_t length;
	};
	const Dblist table[] = {
		g_db2, sizeof(g_db2) / sizeof(g_db2[0]),
		g_db3, sizeof(g_db3) / sizeof(g_db3[0]),
		g_db4, sizeof(g_db4) / sizeof(g_db4[0]),
		g_db5, sizeof(g_db5) / sizeof(g_db5[0]),
		g_db6, sizeof(g_db6) / sizeof(g_db6[0]),
	};

	if (dbInfo.loaded == true && dbInfo.type == dbType::EXACT_VALUES) {
		for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); ++i) {
			for (size_t k = 0; k < table[i].length; ++k) {
				if (table[i].slices[k] != nullptr) {
					free(table[i].slices[k]);
					table[i].slices[k] = nullptr;
				}
			}
		}
		dbInfo.loaded = false;
	}
}


void init_g_piece_counts_to_local_slice(void)
{				
	/**********************************************************/
	/* [white kings][white checkers][red kings][red checkers] */
	/**********************************************************/
	memset( g_piece_counts_to_local_slice, 0, sizeof(g_piece_counts_to_local_slice) );
	
	/***********************************************/
	/*                    DB 02                    */
	/***********************************************/
	g_piece_counts_to_local_slice[1][0][1][0] = k_WR;
	g_piece_counts_to_local_slice[0][1][1][0] = k_wR;
	g_piece_counts_to_local_slice[1][0][0][1] = k_Wr;
	g_piece_counts_to_local_slice[0][1][0][1] = k_wr;
	
	/***********************************************/
	/*                    DB 03                    */
	/***********************************************/
	g_piece_counts_to_local_slice[2][0][1][0] = k_WWR;
	g_piece_counts_to_local_slice[1][0][2][0] = k_RRW;
	g_piece_counts_to_local_slice[1][1][1][0] = k_WwR;
	g_piece_counts_to_local_slice[1][0][1][1] = k_RrW;
	g_piece_counts_to_local_slice[0][2][1][0] = k_wwR;
	g_piece_counts_to_local_slice[1][0][0][2] = k_rrW;
	g_piece_counts_to_local_slice[2][0][0][1] = k_WWr;
	g_piece_counts_to_local_slice[0][1][2][0] = k_RRw;
	g_piece_counts_to_local_slice[1][1][0][1] = k_Wwr;
	g_piece_counts_to_local_slice[0][1][1][1] = k_Rrw;
	g_piece_counts_to_local_slice[0][2][0][1] = k_wwr;
	g_piece_counts_to_local_slice[0][1][0][2] = k_rrw;

	/***********************************************/
	/*                    DB 04                    */
	/***********************************************/	
	g_piece_counts_to_local_slice[2][0][2][0] = k_WWRR;
	g_piece_counts_to_local_slice[2][0][1][1] = k_WWRr;
	g_piece_counts_to_local_slice[2][0][0][2] = k_WWrr;
	g_piece_counts_to_local_slice[1][1][2][0] = k_WwRR;
	g_piece_counts_to_local_slice[1][1][1][1] = k_WwRr;
	g_piece_counts_to_local_slice[1][1][0][2] = k_Wwrr;
	g_piece_counts_to_local_slice[0][2][2][0] = k_wwRR;
	g_piece_counts_to_local_slice[0][2][1][1] = k_wwRr;
	g_piece_counts_to_local_slice[0][2][0][2] = k_wwrr;

	g_piece_counts_to_local_slice[3][0][1][0] = k_WWWR;
	g_piece_counts_to_local_slice[3][0][0][1] = k_WWWr;
	g_piece_counts_to_local_slice[2][1][1][0] = k_WWwR;
	g_piece_counts_to_local_slice[2][1][0][1] = k_WWwr;
	g_piece_counts_to_local_slice[1][2][1][0] = k_WwwR;
	g_piece_counts_to_local_slice[1][2][0][1] = k_Wwwr;
	g_piece_counts_to_local_slice[0][3][1][0] = k_wwwR;
	g_piece_counts_to_local_slice[0][3][0][1] = k_wwwr;
		
	g_piece_counts_to_local_slice[1][0][3][0] = k_RRRW;
	g_piece_counts_to_local_slice[0][1][3][0] = k_RRRw;
	g_piece_counts_to_local_slice[1][0][2][1] = k_RRrW;
	g_piece_counts_to_local_slice[0][1][2][1] = k_RRrw;
	g_piece_counts_to_local_slice[1][0][1][2] = k_RrrW;
	g_piece_counts_to_local_slice[0][1][1][2] = k_Rrrw;
	g_piece_counts_to_local_slice[1][0][0][3] = k_rrrW;
	g_piece_counts_to_local_slice[0][1][0][3] = k_rrrw;
	
	/***********************************************/
	/*                    DB 05                    */
	/***********************************************/
	g_piece_counts_to_local_slice[3][0][2][0] = k_WWWRR;
	g_piece_counts_to_local_slice[2][0][3][0] = k_RRRWW;
	g_piece_counts_to_local_slice[2][1][2][0] = k_WWwRR;
	g_piece_counts_to_local_slice[2][0][2][1] = k_RRrWW;
	g_piece_counts_to_local_slice[1][2][2][0] = k_WwwRR;
	g_piece_counts_to_local_slice[2][0][1][2] = k_RrrWW;
	g_piece_counts_to_local_slice[0][3][2][0] = k_wwwRR;
	g_piece_counts_to_local_slice[2][0][0][3] = k_rrrWW;
	g_piece_counts_to_local_slice[3][0][1][1] = k_WWWRr;
	g_piece_counts_to_local_slice[1][1][3][0] = k_RRRWw;
	g_piece_counts_to_local_slice[2][1][1][1] = k_WWwRr;
	g_piece_counts_to_local_slice[1][1][2][1] = k_RRrWw;
	g_piece_counts_to_local_slice[1][2][1][1] = k_WwwRr;
	g_piece_counts_to_local_slice[1][1][1][2] = k_RrrWw;
	g_piece_counts_to_local_slice[0][3][1][1] = k_wwwRr;
	g_piece_counts_to_local_slice[1][1][0][3] = k_rrrWw;
	g_piece_counts_to_local_slice[3][0][0][2] = k_WWWrr;
	g_piece_counts_to_local_slice[0][2][3][0] = k_RRRww;
	g_piece_counts_to_local_slice[2][1][0][2] = k_WWwrr;
	g_piece_counts_to_local_slice[0][2][2][1] = k_RRrww;
	g_piece_counts_to_local_slice[1][2][0][2] = k_Wwwrr;
	g_piece_counts_to_local_slice[0][2][1][2] = k_Rrrww;
	g_piece_counts_to_local_slice[0][3][0][2] = k_wwwrr;
	g_piece_counts_to_local_slice[0][2][0][3] = k_rrrww;

	/***********************************************/
	/*                    DB 06                    */
	/***********************************************/
	g_piece_counts_to_local_slice[3][0][3][0] = k_WWWRRR;
	g_piece_counts_to_local_slice[2][1][3][0] = k_WWwRRR;
	g_piece_counts_to_local_slice[1][2][3][0] = k_WwwRRR;
	g_piece_counts_to_local_slice[0][3][3][0] = k_wwwRRR;

	g_piece_counts_to_local_slice[3][0][2][1] = k_WWWRRr;
	g_piece_counts_to_local_slice[2][1][2][1] = k_WWwRRr;
	g_piece_counts_to_local_slice[1][2][2][1] = k_WwwRRr;
	g_piece_counts_to_local_slice[0][3][2][1] = k_wwwRRr;
		
	g_piece_counts_to_local_slice[3][0][1][2] = k_WWWRrr;
	g_piece_counts_to_local_slice[2][1][1][2] = k_WWwRrr;
	g_piece_counts_to_local_slice[1][2][1][2] = k_WwwRrr;
	g_piece_counts_to_local_slice[0][3][1][2] = k_wwwRrr;
		
	g_piece_counts_to_local_slice[3][0][0][3] = k_WWWrrr;
	g_piece_counts_to_local_slice[2][1][0][3] = k_WWwrrr;
	g_piece_counts_to_local_slice[1][2][0][3] = k_Wwwrrr;
	g_piece_counts_to_local_slice[0][3][0][3] = k_wwwrrr;
}

// returns 1 on success, 0 on failure
int LoadSingleDatabase( const char *filename, unsigned char** sliceData, int bytes_needed_for_db_slice )
{
	char fullFilename[512];
	sprintf( fullFilename, "%s/%s", checkerBoard.db_path, filename );
	assert( *sliceData == NULL );

	FILE *db_file = fopen( fullFilename ,"rb");
	if ( db_file ) 
	{
		// Allocate memory for the slice
		*sliceData = (unsigned char *)malloc(bytes_needed_for_db_slice);

		unsigned int unitsRead = (unsigned int)fread( *sliceData, bytes_needed_for_db_slice, 1, db_file );
		fclose( db_file );

		assert( unitsRead == 1 );
		return unitsRead;
	}
	return 0;
}

// Load Ed Trice's endgame database
bool LoadEdsDatabase()
{
	int numLoaded = 0;
	init_g_piece_counts_to_local_slice();

	/***************************************************/
	/* 2-piece database                                */
	/***************************************************/
	numLoaded += LoadSingleDatabase( "db_02_(1K0C_1K0C)", &g_db2[k_WR], 992 );

	numLoaded += LoadSingleDatabase( "db_02_(1K0C_0K1C)", &g_db2[k_Wr], 868 );
	numLoaded += LoadSingleDatabase( "db_02_(0K1C_1K0C)", &g_db2[k_wR], 868 );

	numLoaded += LoadSingleDatabase( "db_02_(0K1C_0K1C)", &g_db2[k_wr], 760 );
	
	/***************************************************/
	/* 3-piece database                                */
	/***************************************************/

	numLoaded += LoadSingleDatabase( "db_03_(2K0C_1K0C)", &g_db3[k_WWR], 14880 );
	numLoaded += LoadSingleDatabase( "db_03_(1K0C_2K0C)", &g_db3[k_RRW], 14880 );

	numLoaded += LoadSingleDatabase( "db_03_(2K0C_0K1C)", &g_db3[k_WWr], 13020 );
	numLoaded += LoadSingleDatabase( "db_03_(0K1C_2K0C)", &g_db3[k_RRw], 13020 );

	numLoaded += LoadSingleDatabase( "db_03_(1K1C_1K0C)", &g_db3[k_WwR], 26040 );
	numLoaded += LoadSingleDatabase( "db_03_(1K0C_1K1C)", &g_db3[k_RrW], 26040 );

	numLoaded += LoadSingleDatabase( "db_03_(1K1C_0K1C)", &g_db3[k_Wwr], 22800 );
	numLoaded += LoadSingleDatabase( "db_03_(0K1C_1K1C)", &g_db3[k_Rrw], 22800 );

	numLoaded += LoadSingleDatabase( "db_03_(0K2C_1K0C)", &g_db3[k_wwR], 11340 );
	numLoaded += LoadSingleDatabase( "db_03_(1K0C_0K2C)", &g_db3[k_rrW], 11340 );

	numLoaded += LoadSingleDatabase( "db_03_(0K2C_0K1C)", &g_db3[k_wwr], 9936 );
	numLoaded += LoadSingleDatabase( "db_03_(0K1C_0K2C)", &g_db3[k_rrw], 9936 );

	/***************************************************/
	/* 4-piece database	                               */
	/***************************************************/

	numLoaded += LoadSingleDatabase( "db_04_(2K0C_2K0C)", &g_db4[k_WWRR], 215760 );

	numLoaded += LoadSingleDatabase( "db_04_(2K0C_1K1C)", &g_db4[k_WWRr], 377580 );
	numLoaded += LoadSingleDatabase( "db_04_(1K1C_2K0C)", &g_db4[k_WwRR], 377580 );

	numLoaded += LoadSingleDatabase( "db_04_(2K0C_0K2C)", &g_db4[k_WWrr], 164430 );
	numLoaded += LoadSingleDatabase( "db_04_(0K2C_2K0C)", &g_db4[k_wwRR], 164430 );

	numLoaded += LoadSingleDatabase( "db_04_(1K1C_1K1C)", &g_db4[k_WwRr], 661200 );

	numLoaded += LoadSingleDatabase( "db_04_(1K1C_0K2C)", &g_db4[k_Wwrr], 288144 );
	numLoaded += LoadSingleDatabase( "db_04_(0K2C_1K1C)", &g_db4[k_wwRr], 288144 );

	numLoaded += LoadSingleDatabase( "db_04_(0K2C_0K2C)", &g_db4[k_wwrr], 125664 );

	numLoaded += LoadSingleDatabase( "db_04_(3K0C_1K0C)", &g_db4[k_WWWR], 143840 );
	numLoaded += LoadSingleDatabase( "db_04_(1K0C_3K0C)", &g_db4[k_RRRW], 143840 );
		
	numLoaded += LoadSingleDatabase( "db_04_(2K1C_1K0C)", &g_db4[k_WWwR], 377580 );
	numLoaded += LoadSingleDatabase( "db_04_(1K0C_2K1C)", &g_db4[k_RRrW], 377580 );

	numLoaded += LoadSingleDatabase( "db_04_(1K2C_1K0C)", &g_db4[k_WwwR], 328860 );
	numLoaded += LoadSingleDatabase( "db_04_(1K0C_1K2C)", &g_db4[k_RrrW], 328860 );

	numLoaded += LoadSingleDatabase( "db_04_(0K3C_1K0C)", &g_db4[k_wwwR], 95004 );
	numLoaded += LoadSingleDatabase( "db_04_(1K0C_0K3C)", &g_db4[k_rrrW], 95004 );

	numLoaded += LoadSingleDatabase( "db_04_(3K0C_0K1C)", &g_db4[k_WWWr], 125860 );
	numLoaded += LoadSingleDatabase( "db_04_(0K1C_3K0C)", &g_db4[k_RRRw], 125860 );

	numLoaded += LoadSingleDatabase( "db_04_(2K1C_0K1C)", &g_db4[k_WWwr], 330600 );
	numLoaded += LoadSingleDatabase( "db_04_(0K1C_2K1C)", &g_db4[k_RRrw], 330600 );

	numLoaded += LoadSingleDatabase( "db_04_(1K2C_0K1C)", &g_db4[k_Wwwr], 288144 );
	numLoaded += LoadSingleDatabase( "db_04_(0K1C_1K2C)", &g_db4[k_Rrrw], 288144 );

	numLoaded += LoadSingleDatabase( "db_04_(0K3C_0K1C)", &g_db4[k_wwwr], 83304 );
	numLoaded += LoadSingleDatabase( "db_04_(0K1C_0K3C)", &g_db4[k_rrrw], 83304 );

	/***************************************************/
	/* 5-piece database                                */
	/***************************************************/
	
	numLoaded += LoadSingleDatabase( "db_05_(3K0C_2K0C)", &g_db5[k_WWWRR], 2013760 );
	numLoaded += LoadSingleDatabase( "db_05_(2K0C_3K0C)", &g_db5[k_RRRWW], 2013760 );

	numLoaded += LoadSingleDatabase( "db_05_(3K0C_1K1C)", &g_db5[k_WWWRr], 3524080 );
	numLoaded += LoadSingleDatabase( "db_05_(1K1C_3K0C)", &g_db5[k_RRRWw], 3524080 );
			
	numLoaded += LoadSingleDatabase( "db_05_(3K0C_0K2C)", &g_db5[k_WWWrr], 1534680 );
	numLoaded += LoadSingleDatabase( "db_05_(0K2C_3K0C)", &g_db5[k_RRRww], 1534680 );

	numLoaded += LoadSingleDatabase( "db_05_(2K1C_2K0C)", &g_db5[k_WWwRR], 5286120 );
	numLoaded += LoadSingleDatabase( "db_05_(2K0C_2K1C)", &g_db5[k_RRrWW], 5286120 );

	numLoaded += LoadSingleDatabase( "db_05_(2K1C_1K1C)", &g_db5[k_WWwRr], 9256800 );
	numLoaded += LoadSingleDatabase( "db_05_(1K1C_2K1C)", &g_db5[k_RRrWw], 9256800 );

	numLoaded += LoadSingleDatabase( "db_05_(2K1C_0K2C)", &g_db5[k_WWwrr], 4034016 );
	numLoaded += LoadSingleDatabase( "db_05_(0K2C_2K1C)", &g_db5[k_RRrww], 4034016 );

	numLoaded += LoadSingleDatabase( "db_05_(1K2C_2K0C)", &g_db5[k_WwwRR], 4604040 );
	numLoaded += LoadSingleDatabase( "db_05_(2K0C_1K2C)", &g_db5[k_RrrWW], 4604040 );

	numLoaded += LoadSingleDatabase( "db_05_(1K2C_1K1C)", &g_db5[k_WwwRr], 8068032 );
	numLoaded += LoadSingleDatabase( "db_05_(1K1C_1K2C)", &g_db5[k_RrrWw], 8068032 );

	numLoaded += LoadSingleDatabase( "db_05_(1K2C_0K2C)", &g_db5[k_Wwwrr], 3518592 );
	numLoaded += LoadSingleDatabase( "db_05_(0K2C_1K2C)", &g_db5[k_Rrrww], 3518592 );

	numLoaded += LoadSingleDatabase( "db_05_(0K3C_2K0C)", &g_db5[k_wwwRR], 1330056 );
	numLoaded += LoadSingleDatabase( "db_05_(2K0C_0K3C)", &g_db5[k_rrrWW], 1330056 );
						
	numLoaded += LoadSingleDatabase( "db_05_(0K3C_1K1C)", &g_db5[k_wwwRr], 2332512 );
	numLoaded += LoadSingleDatabase( "db_05_(1K1C_0K3C)", &g_db5[k_rrrWw], 2332512 );

	numLoaded += LoadSingleDatabase( "db_05_(0K3C_0K2C)", &g_db5[k_wwwrr], 1018056 );
	numLoaded += LoadSingleDatabase( "db_05_(0K2C_0K3C)", &g_db5[k_rrrww], 1018056 );
			
	/***************************************************/
	/* 6-piece database                                */
	/***************************************************/

	numLoaded += LoadSingleDatabase( "db_06_(3K0C_3K0C)", &g_db6[k_WWWRRR], 18123840 );

	numLoaded += LoadSingleDatabase( "db_06_(2K1C_3K0C)", &g_db6[k_WWwRRR], 47575080 );
	numLoaded += LoadSingleDatabase( "db_06_(3K0C_2K1C)", &g_db6[k_WWWRRr], 47575080 );

	numLoaded += LoadSingleDatabase( "db_06_(1K2C_3K0C)", &g_db6[k_WwwRRR], 41436360 );
	numLoaded += LoadSingleDatabase( "db_06_(3K0C_1K2C)", &g_db6[k_WWWRrr], 41436360 );

	numLoaded += LoadSingleDatabase( "db_06_(0K3C_3K0C)", &g_db6[k_wwwRRR], 11970504 );
	numLoaded += LoadSingleDatabase( "db_06_(3K0C_0K3C)", &g_db6[k_WWWrrr], 11970504 );

	numLoaded += LoadSingleDatabase( "db_06_(2K1C_2K1C)", &g_db6[k_WWwRRr], 124966800 );
				
	numLoaded += LoadSingleDatabase( "db_06_(1K2C_2K1C)", &g_db6[k_WwwRRr], 108918432 );
	numLoaded += LoadSingleDatabase( "db_06_(2K1C_1K2C)", &g_db6[k_WWwRrr], 108918432 );

	numLoaded += LoadSingleDatabase( "db_06_(0K3C_2K1C)", &g_db6[k_wwwRRr], 31488912 );
	numLoaded += LoadSingleDatabase( "db_06_(2K1C_0K3C)", &g_db6[k_WWwrrr], 31488912 );

	numLoaded += LoadSingleDatabase( "db_06_(1K2C_1K2C)", &g_db6[k_WwwRrr], 95001984 );

	numLoaded += LoadSingleDatabase( "db_06_(0K3C_1K2C)", &g_db6[k_wwwRrr], 27487512 );
	numLoaded += LoadSingleDatabase( "db_06_(1K2C_0K3C)", &g_db6[k_Wwwrrr], 27487512 );

	numLoaded += LoadSingleDatabase( "db_06_(0K3C_0K3C)", &g_db6[k_wwwrrr], 7959904 );

	/* Require all 2 through 6 piece slices to be loaded. */
	if ( numLoaded == 81 )
		return true;

	return false;
}

// Ed Trice's Database
/*********************************************************************************************************************/
/*                                                                                                                   */
/* Ed Trice September 6, 2009: Explanation of database "flopping"                                                    */
/*                                                                                                                   */
/* Here, and elsewhere, you will see many "33-" parameters being fed into the indexing functions. This is due to the */
/* nature of the indexing function design. In this specific example, there is an indexing function for the material  */
/* distribution of 2 white kings vs. 1 red king, but there is NOT one for 2 red kings vs. 1 white king. You can see  */
/* why this is the case: there would be "double the work."                                                           */
/*                                                                                                                   */
/* To get around this, the indexing functions were designed conforming to the following specification:               */
/*                                                                                                                   */
/* - The constant W indicates a White King    is in the corresponding location.                                      */
/* - The constant w indicates a White Checker is in the corresponding location.                                      */
/* - The constant R indicates a Red King      is in the corresponding location.                                      */
/* - The constant r indicates a Red Checker   is in the corresponding location.                                      */
/*                                                                                                                   */
/* Example: WWwRrr is a 6-piece database slice with 2 white kings + 1 white checker vs. 1 red king + 2 red checkers  */
/*                                                                                                                   */
/* ALL DATABASE ENTRIES ARE FOR WHITE TO MOVE. To look up a red-to-move position, the board must be transformed to   */
/* its white-to-move equivalent.                                                                                     */
/*                                                                                                                   */
/* Example: White kings on square 24 and 10, white checkers on square 18,  Red king on 11, red checkers on 3 and 16. */
/*                                                                                                                   */
/* What is the result if it is red to move? (square 32 = top left, square 1 = bottom right)                          */
/*                                                                                                                   */
/* -----------------------------------------------------------------                                                 */
/* |       |#######|       |#######|       |#######|       |#######|                                                 */
/* |       |#######|       |#######|       |#######|       |#######|                                                 */
/* |       |#######|       |#######|       |#######|       |#######|                                                 */
/* -----------------------------------------------------------------                                                 */
/* |#######|       |#######|       |#######|       |#######|       |                                                 */
/* |#######|       |#######|       |#######|       |#######|       |                                                 */
/* |#######|       |#######|       |#######|       |#######|       |                                                 */
/* -----------------------------------------------------------------                                                 */
/* |       |#######|       |#######|       |#######|       |#######|                                                 */
/* |       |## W ##|       |#######|       |#######|       |#######|                                                 */
/* |       |#######|       |#######|       |#######|       |#######|                                                 */
/* -----------------------------------------------------------------                                                 */
/* |#######|       |#######|       |#######|       |#######|       |                                                 */
/* |#######|       |#######|       |## w ##|       |#######|       |                                                 */
/* |#######|       |#######|       |#######|       |#######|       |                                                 */				
/* -----------------------------------------------------------------                                                 */
/* |       |#######|       |#######|       |#######|       |#######|                                                 */
/* |       |## r ##|       |#######|       |#######|       |#######|                                                 */
/* |       |#######|       |#######|       |#######|       |#######|                                                 */
/* -----------------------------------------------------------------                                                 */
/* |#######|       |#######|       |#######|       |#######|       |                                                 */
/* |#######|       |## R ##|       |## W ##|       |#######|       |                                                 */
/* |#######|       |#######|       |#######|       |#######|       |                                                 */
/* -----------------------------------------------------------------                                                 */
/* |       |#######|       |#######|       |#######|       |#######|                                                 */
/* |       |#######|       |#######|       |#######|       |#######|                                                 */
/* |       |#######|       |#######|       |#######|       |#######|                                                 */
/* -----------------------------------------------------------------                                                 */
/* |#######|       |#######|       |#######|       |#######|       |                                                 */
/* |#######|       |## r ##|       |#######|       |#######|       |                                                 */
/* |#######|       |#######|       |#######|       |#######|       |                                                 */
/* -----------------------------------------------------------------                                                 */	
/*                                                                                                                   */
/* This is slice k_WWwRrr with red to move. The W pieces above are the white kings on 10 and 24. The lower case w is */		
/* the checker on square 18. The R is the red king on square 11. The lower case r represent the 2 red checkers, on   */	
/* squares 3 and 16. The problem is, there is no "red to move" information in the databases. The position must be    */		
/* converted into a "white to move" position.                                                                        */
/*                                                                                                                   */
/* This is done by changing the colors of the pieces, and placing them on 33 - their current square numbers.         */
/*                                                                                                                   */
/* W[0] = 10, W[1] = 24, w[0] = 18:    R[0] = 11, r[0] = 3, r[1] = 16.                                               */
/*                                                                                                                   */
/* Note: in this case, we go from k_WWwRrr to k_WwwRRr.                                                              */
/*                                                                                                                   */
/* New W[0] = 33 - old R[0] = 33 - 11 = 22.                                                                          */
/* New w[0] = 33 - old r[1] = 33 - 16 = 17.   (why we are doing 33 - r[1] and not 33 - r[0] is explained below.)     */
/* New w[1] = 33 - old r[0] = 33 -  3 = 30.                                                                          */
/*                                                                                                                   */
/* New R[0] = 33 - old W[1] = 33 - 24 =  9.   (why we are doing 33 - W[1] and not 33 - W[0] is explained below.)     */ 
/* New R[1] = 33 - old W[0] = 33 - 10 = 23.                                                                          */ 
/* New r[0] = 33 - old w[0] = 33 - 18 = 15.                                                                          */
/*                                                                                                                   */
/* The ORDER in which the location of these pieces reside is very important! Always "pass" to the indexing function  */
/* the same pieces that occur in the same order as the "k_" constant. So "k_RRW" will expect the 2 red kings first   */
/* to be followed by the square for the white king.                                                                  */
/*                                                                                                                   */
/* When MULTIPLE pieces of the same type are passed, they must be in ascending order! For example, in the slice ...  */
/*                                                                                                                   */
/* k_WWWRRr: the square of the 1st white king < the square of the 2nd white king < the square of the 3rd white king  */
/*           the square of the 1st red king   < the square of the 2nd red king   			                         */
/*                                                                                                                   */
/* k_RrrWw:  the square of the 1st red checker < the square of the 2nd red checker                                   */
/*                                                                                                                   */	
/* Therefore when looking up the index for a red-dominated slice (such as K_RRW below) we have to "switch" locations */
/* of the red pieces, pretending they are "white" for the indexing function, and vice versa.                         */	
/*                                                                                                                   */
/* Because the pieces are SORTED, when you have 2 red kings, you DON'T pass in 33 - R[0] and 33 - R[1] (!!!)         */
/*                                                                                                                   */
/* Since R[0] < R[1], 33 - R[0] > 33 - R[1]. Thefore, you need to pass the higher subindex (in this case, [1]) prior */
/* to passing the lower subindex (in this case, [0]).  Likewise, for 3 red kings or checkers, you would need to pass */
/* them in as 33 - [2], 33 - [1], and 33 - [0], whether they are the R[] or r[] pieces.                              */
/*                                                                                                                   */
/* So, the red to move example diagram above will be "flopped" to the white to move diagram shown below:             */
/*                                                                                                                   */
/* -----------------------------------------------------------------                                                 */
/* |       |#######|       |#######|       |#######|       |#######|                                                 */
/* |       |#######|       |#######|       |## w ##|       |#######|                                                 */
/* |       |#######|       |#######|       |#######|       |#######|                                                 */
/* -----------------------------------------------------------------                                                 */
/* |#######|       |#######|       |#######|       |#######|       |                                                 */
/* |#######|       |#######|       |#######|       |#######|       |                                                 */
/* |#######|       |#######|       |#######|       |#######|       |                                                 */
/* -----------------------------------------------------------------                                                 */
/* |       |#######|       |#######|       |#######|       |#######|                                                 */
/* |       |#######|       |## R ##|       |## W ##|       |#######|                                                 */
/* |       |#######|       |#######|       |#######|       |#######|                                                 */
/* -----------------------------------------------------------------                                                 */
/* |#######|       |#######|       |#######|       |#######|       |                                                 */
/* |#######|       |#######|       |#######|       |## w ##|       |                                                 */
/* |#######|       |#######|       |#######|       |#######|       |                                                 */				
/* -----------------------------------------------------------------                                                 */
/* |       |#######|       |#######|       |#######|       |#######|                                                 */
/* |       |#######|       |## r ##|       |#######|       |#######|                                                 */
/* |       |#######|       |#######|       |#######|       |#######|                                                 */
/* -----------------------------------------------------------------                                                 */
/* |#######|       |#######|       |#######|       |#######|       |                                                 */
/* |#######|       |#######|       |#######|       |## R ##|       |                                                 */
/* |#######|       |#######|       |#######|       |#######|       |                                                 */
/* -----------------------------------------------------------------                                                 */
/* |       |#######|       |#######|       |#######|       |#######|                                                 */
/* |       |#######|       |#######|       |#######|       |#######|                                                 */
/* |       |#######|       |#######|       |#######|       |#######|                                                 */
/* -----------------------------------------------------------------                                                 */
/* |#######|       |#######|       |#######|       |#######|       |                                                 */
/* |#######|       |#######|       |#######|       |#######|       |                                                 */
/* |#######|       |#######|       |#######|       |#######|       |                                                 */
/* -----------------------------------------------------------------                                                 */	
/*                                                                                                                   */	
/* This white to move position is a loss in 66 plies, with 22-26 being the best move for white.                      */	
/* Therefore, the red to move position above is a loss in 66 plies also, with 11-7 being the best defense.           */	
/*                                                                                                                   */								
/*********************************************************************************************************************/

void inline orderChar( unsigned char &a, unsigned char &b )
{
	if ( a < b ) 
	{
		unsigned char temp = a;
		a = b;
		b = temp;
	}
}

int inline AddPieces( unsigned char piece[6], int &piece_count, uint32_t PieceBits )
{
	int start_count = piece_count;
	while ( PieceBits ) 
	{
		uint32_t sq = FindLowBit( PieceBits );
		PieceBits &= ~S[sq];
		
		piece[ piece_count++ ] = (unsigned char)(FlipSqX(sq)+1);
	}
	int numType = piece_count - start_count;

	if ( numType > 1 ) // unfortunately this is necessary because my Gui Checkers bitboard has flipped X values
		orderChar( piece[ piece_count-1 ], piece[ piece_count-2] ); 
	if ( numType > 2 ) {
		orderChar( piece[ piece_count-2 ], piece[ piece_count-3] ); 
		orderChar( piece[ piece_count-1 ], piece[ piece_count-2] ); 
	}
	return numType;
}

int inline AddPiecesFlipped( unsigned char piece[6], int &piece_count, uint32_t PieceBits )
{
	int start_count = piece_count;
	while ( PieceBits ) 
	{
		uint32_t sq = FindHighBit( PieceBits );
		PieceBits &= ~S[sq];
		
		piece[ piece_count++ ] = (unsigned char)(32- FlipSqX(sq));
	}
	int numType = piece_count - start_count;

	if ( numType > 1 )
		orderChar( piece[ piece_count-1 ], piece[ piece_count-2] );
	if ( numType > 2 ) {
		orderChar( piece[ piece_count-2 ], piece[ piece_count-3] ); 
		orderChar( piece[ piece_count-1 ], piece[ piece_count-2] ); 
	}
	return numType;
}

int QueryEdsDatabase( const Board &Board, int ahead )
{
	unsigned char which_db, which_slice;
	unsigned long long index_function_value;
	unsigned char distance_to_win_or_lose_or_possibly_draw = DB_UNKNOWN_THAT_FITS_INTO_8_BITS;
	unsigned char white_king_count, white_checker_count, red_king_count, red_checker_count;
	unsigned char piece[6];
	int piece_count = 0;

	// Arrange pieces in an array with the following properties :
	// 1. dominant side first (white first if equal)
	// 2. kings first
	// 3. lower square first, 
	if ( Board.sideToMove == WHITE )
	{
		if ( Board.numPieces[WHITE] >= Board.numPieces[BLACK])
		{
			white_king_count = AddPieces ( piece, piece_count, Board.Bitboards.P[WHITE] & Board.Bitboards.K );
			white_checker_count = AddPieces ( piece, piece_count, Board.Bitboards.P[WHITE] & ~Board.Bitboards.K );
			red_king_count = AddPieces ( piece, piece_count, Board.Bitboards.P[BLACK] & Board.Bitboards.K );
			red_checker_count = AddPieces ( piece, piece_count, Board.Bitboards.P[BLACK] & ~Board.Bitboards.K );
		}
		else
		{
			red_king_count = AddPieces ( piece, piece_count, Board.Bitboards.P[BLACK] & Board.Bitboards.K );
			red_checker_count = AddPieces ( piece, piece_count, Board.Bitboards.P[BLACK] & ~Board.Bitboards.K );
			white_king_count = AddPieces ( piece, piece_count, Board.Bitboards.P[WHITE] & Board.Bitboards.K );
			white_checker_count = AddPieces ( piece, piece_count, Board.Bitboards.P[WHITE] & ~Board.Bitboards.K );
		}
	}
	else
	{
		// need to "flop" everything if red to move
		const CheckerBitboards& Bb = Board.Bitboards;
		if ( Board.numPieces[WHITE] > Board.numPieces[BLACK])
		{
			red_king_count = AddPiecesFlipped ( piece, piece_count, Bb.P[WHITE] & Bb.K );
			red_checker_count = AddPiecesFlipped ( piece, piece_count, Bb.P[WHITE] & ~Bb.K );
			white_king_count = AddPiecesFlipped ( piece, piece_count, Bb.P[BLACK] & Bb.K );
			white_checker_count = AddPiecesFlipped ( piece, piece_count, Bb.P[BLACK] & ~Bb.K );
		}
		else
		{
			white_king_count = AddPiecesFlipped ( piece, piece_count, Bb.P[BLACK] & Bb.K );
			white_checker_count = AddPiecesFlipped ( piece, piece_count, Bb.P[BLACK] & ~Bb.K );
			red_king_count = AddPiecesFlipped ( piece, piece_count, Bb.P[WHITE] & Bb.K );
			red_checker_count = AddPiecesFlipped ( piece, piece_count, Bb.P[WHITE] & ~Bb.K );
		}
	}

	assert( Board.numPieces[WHITE] + Board.numPieces[BLACK] == white_king_count + white_checker_count + red_king_count + red_checker_count );

	which_db = Board.numPieces[WHITE] + Board.numPieces[BLACK];
	which_slice = (unsigned char)g_piece_counts_to_local_slice[white_king_count][white_checker_count][red_king_count][red_checker_count];

	// make sure this slice is defined for one of our databases
	if ( which_slice == 0 )
		return INVALID_DB_VALUE;

	// Choose database based on how many pieces are on the board
	switch(which_db) 
	{
		case 2: 
			index_function_value = get_02_piece_index_for_slice( which_slice, piece[0], piece[1] );

			if ( g_db2[which_slice] ) 
				distance_to_win_or_lose_or_possibly_draw = g_db2[which_slice][index_function_value];
		break;
		
		case 3:
			index_function_value = get_03_piece_index_for_slice( which_slice, piece[0], piece[1], piece[2] );

			if ( g_db3[which_slice] ) 
				distance_to_win_or_lose_or_possibly_draw = g_db3[which_slice][index_function_value];
		break;
		
		case 4:
			index_function_value = get_04_piece_index_for_slice( which_slice, piece[0], piece[1], piece[2], piece[3] );

			if ( g_db4[which_slice] ) 
				distance_to_win_or_lose_or_possibly_draw = g_db4[which_slice][index_function_value];
		break;
		
		case 5:
			index_function_value = get_05_piece_index_for_slice( which_slice, piece[0], piece[1], piece[2], piece[3], piece[4] );

			if ( g_db5[which_slice] ) 
				distance_to_win_or_lose_or_possibly_draw = g_db5[which_slice][index_function_value];
		break;
		
		case 6:
			index_function_value = get_06_piece_index_for_slice( which_slice, piece[0], piece[1], piece[2], piece[3], piece[4], piece[5] );

			if ( g_db6[which_slice] ) 
				distance_to_win_or_lose_or_possibly_draw = g_db6[which_slice][index_function_value];
		break;
	}

	/**********************************************************************************************/
	/*                                                                                            */
	/* Ed Trice, September 6, 2009                                                                */
	/*                                                                                            */			
	/* The databases return data from the perspective of white to move.                           */
	/*                                                                                            */
	/* Always check to see if the result is a DRAW first by comparing                             */
	/* to DB_DRAW_THAT_FITS_INTO_8_BITS (which is the value 254).                                 */
	/*                                                                                            */
	/* "254" is the only even value that is NOT a loss!                                           */
	/*                                                                                            */
	/* All ODD  values are wins for the white side to move.                                       */
	/* All EVEN values are losses for the white side to move.                                     */
	/*                                                                                            */
	/* Since these are EXACT values, you should do the following:                                 */
	/*                                                                                            */
	/* 1. if(distance_to_win_or_lose_or_possibly_draw == DB_DRAW_THAT_FITS_INTO_8_BITS) return 0; */
	/*                                                                                            */
	/* 2. if(distance_to_win_or_lose_or_possibly_draw % 2) then you have a white win, since it is */
	/*    an odd number. return the value:                                                        */
	/*                                                                                            */
	/*                   2001 - distance_to_win_or_lose_or_possibly_draw - ahead                  */
	/*                                                                                            */
	/*    if you maximum good scores for white.                                                   */
	/*                                                                                            */
	/* 3. if(distance_to_win_or_lose_or_possibly_draw % 2 == 0) then you have a white loss, since */
	/*    it is an even number. return the value:                                                 */
	/*                                                                                            */
	/*                   -2001 + distance_to_win_or_lose_or_possibly_draw + ahead                 */
	/*                                                                                            */
	/*    if you want to postpone a white loss for as long as possible.                           */
	/*                                                                                            */
	/**********************************************************************************************/
	if ( distance_to_win_or_lose_or_possibly_draw == DB_UNKNOWN_THAT_FITS_INTO_8_BITS ) 
		return INVALID_DB_VALUE;

	if( distance_to_win_or_lose_or_possibly_draw == DB_DRAW_THAT_FITS_INTO_8_BITS ) return 0;
	
	int eval = 2001 - distance_to_win_or_lose_or_possibly_draw - ahead;

	if ( Board.sideToMove != WHITE ) // We flipped boards, flip the eval too
		eval = -eval;

	if ( (distance_to_win_or_lose_or_possibly_draw & 1) == 0 ) // even number == opponent win
		return -eval;
	else
		return eval;
}

/**********************************************/
/*                                            */
/* Ed Trice, August 13, 2009                  */
/*                                            */
/* Added the following piece configurations:  */
/*                                            */
/* 3 versus 1        1 versus 3               */
/* 3 versus 2        2 versus 3               */
/*          3 versus 3                        */
/*                                            */
/* All databases with 3 and fewer pieces now  */
/* have true distance to win information, not */
/* just "win loss draw" bitbases.             */
/*                                            */
/**********************************************/

void InitializeEdsDatabases( SDatabaseInfo &dbInfo )
{
	// fill out info structure
	dbInfo.numPieces = 6;
	dbInfo.numBlack = 3;
	dbInfo.numWhite = 3;
	dbInfo.type = dbType::EXACT_VALUES;

	// Load the databases
	dbInfo.loaded = LoadEdsDatabase();
}