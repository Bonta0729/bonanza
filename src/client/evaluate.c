#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include "shogi.h"

#define PcPcOnSqAny(k,i,j) ( i >= j ? PcPcOnSq(k,i,j) : PcPcOnSq(k,j,i) )

static int CONV doacapt( const tree_t * restrict ptree, int pc, int turn,
			 int hand_index, const int list0[52],
			 const int list1[52], int nlist );
static int CONV doapc( const tree_t * restrict ptree, int pc, int sq,
		       const int list0[52], const int list1[52], int nlist );
static int CONV ehash_probe( uint64_t current_key, unsigned int hand_b,
			     int * restrict pscore );
static void CONV ehash_store( uint64_t key, unsigned int hand_b, int score );
static int CONV calc_difference( const tree_t * restrict ptree, int ply,
				 int turn, int list0[52], int list1[52],
				 int * restrict pscore );
static int CONV make_list( const tree_t * restrict ptree,
			   int * restrict pscore,
			   int list0[52], int list1[52] );

#if defined(ENTERING_KING_BONUS)
static int ENTERING_KING_score(const tree_t * restrict ptree);
#endif

#if defined(INANIWA_SHIFT)
static int inaniwa_score( const tree_t * restrict ptree );
#endif

int CONV
eval_material( const tree_t * restrict ptree )
{
  int material, itemp;

  itemp     = PopuCount( BB_BPAWN )   + (int)I2HandPawn( HAND_B );
  itemp    -= PopuCount( BB_WPAWN )   + (int)I2HandPawn( HAND_W );
  material  = itemp * p_value[15+pawn];

  itemp     = PopuCount( BB_BLANCE )  + (int)I2HandLance( HAND_B );
  itemp    -= PopuCount( BB_WLANCE )  + (int)I2HandLance( HAND_W );
  material += itemp * p_value[15+lance];

  itemp     = PopuCount( BB_BKNIGHT ) + (int)I2HandKnight( HAND_B );
  itemp    -= PopuCount( BB_WKNIGHT ) + (int)I2HandKnight( HAND_W );
  material += itemp * p_value[15+knight];

  itemp     = PopuCount( BB_BSILVER ) + (int)I2HandSilver( HAND_B );
  itemp    -= PopuCount( BB_WSILVER ) + (int)I2HandSilver( HAND_W );
  material += itemp * p_value[15+silver];

  itemp     = PopuCount( BB_BGOLD )   + (int)I2HandGold( HAND_B );
  itemp    -= PopuCount( BB_WGOLD )   + (int)I2HandGold( HAND_W );
  material += itemp * p_value[15+gold];

  itemp     = PopuCount( BB_BBISHOP ) + (int)I2HandBishop( HAND_B );
  itemp    -= PopuCount( BB_WBISHOP ) + (int)I2HandBishop( HAND_W );
  material += itemp * p_value[15+bishop];

  itemp     = PopuCount( BB_BROOK )   + (int)I2HandRook( HAND_B );
  itemp    -= PopuCount( BB_WROOK )   + (int)I2HandRook( HAND_W );
  material += itemp * p_value[15+rook];

  itemp     = PopuCount( BB_BPRO_PAWN );
  itemp    -= PopuCount( BB_WPRO_PAWN );
  material += itemp * p_value[15+pro_pawn];

  itemp     = PopuCount( BB_BPRO_LANCE );
  itemp    -= PopuCount( BB_WPRO_LANCE );
  material += itemp * p_value[15+pro_lance];

  itemp     = PopuCount( BB_BPRO_KNIGHT );
  itemp    -= PopuCount( BB_WPRO_KNIGHT );
  material += itemp * p_value[15+pro_knight];

  itemp     = PopuCount( BB_BPRO_SILVER );
  itemp    -= PopuCount( BB_WPRO_SILVER );
  material += itemp * p_value[15+pro_silver];

  itemp     = PopuCount( BB_BHORSE );
  itemp    -= PopuCount( BB_WHORSE );
  material += itemp * p_value[15+horse];

  itemp     = PopuCount( BB_BDRAGON );
  itemp    -= PopuCount( BB_WDRAGON );
  material += itemp * p_value[15+dragon];

  return material;
}


int CONV
evaluate( tree_t * restrict ptree, int ply, int turn )
{
  int list0[52], list1[52];
  int nlist, score, sq_bk, sq_wk, k0, k1, l0, l1, i, j, sum;

  assert( 0 < ply );
  ptree->neval_called++;

  if ( ptree->save_eval[ply] != INT_MAX )
    {
      return (int)ptree->save_eval[ply] / FV_SCALE;
    }

 #ifdef ENTERING_KING
    {
	 int ek_b = eking_value_black(ptree), ek_w = eking_value_white(ptree);
	 ptree->save_eking_black[ply] = (ek_b >= 0 ? eking[ek_b] * FV_SCALE : 0);
	 ptree->save_eking_white[ply] = (ek_w >= 0 ? eking[ek_w] * FV_SCALE : 0);
    }
 #endif

  if ( ehash_probe( HASH_KEY, HAND_B, &score ) )
    {
      score                 = turn ? -score : score;
      ptree->save_eval[ply] = score;

      return score / FV_SCALE;
    }

  list0[ 0] = f_hand_pawn   + I2HandPawn(HAND_B);
  list0[ 1] = e_hand_pawn   + I2HandPawn(HAND_W);
  list0[ 2] = f_hand_lance  + I2HandLance(HAND_B);
  list0[ 3] = e_hand_lance  + I2HandLance(HAND_W);
  list0[ 4] = f_hand_knight + I2HandKnight(HAND_B);
  list0[ 5] = e_hand_knight + I2HandKnight(HAND_W);
  list0[ 6] = f_hand_silver + I2HandSilver(HAND_B);
  list0[ 7] = e_hand_silver + I2HandSilver(HAND_W);
  list0[ 8] = f_hand_gold   + I2HandGold(HAND_B);
  list0[ 9] = e_hand_gold   + I2HandGold(HAND_W);
  list0[10] = f_hand_bishop + I2HandBishop(HAND_B);
  list0[11] = e_hand_bishop + I2HandBishop(HAND_W);
  list0[12] = f_hand_rook   + I2HandRook(HAND_B);
  list0[13] = e_hand_rook   + I2HandRook(HAND_W);

  list1[ 0] = f_hand_pawn   + I2HandPawn(HAND_W);
  list1[ 1] = e_hand_pawn   + I2HandPawn(HAND_B);
  list1[ 2] = f_hand_lance  + I2HandLance(HAND_W);
  list1[ 3] = e_hand_lance  + I2HandLance(HAND_B);
  list1[ 4] = f_hand_knight + I2HandKnight(HAND_W);
  list1[ 5] = e_hand_knight + I2HandKnight(HAND_B);
  list1[ 6] = f_hand_silver + I2HandSilver(HAND_W);
  list1[ 7] = e_hand_silver + I2HandSilver(HAND_B);
  list1[ 8] = f_hand_gold   + I2HandGold(HAND_W);
  list1[ 9] = e_hand_gold   + I2HandGold(HAND_B);
  list1[10] = f_hand_bishop + I2HandBishop(HAND_W);
  list1[11] = e_hand_bishop + I2HandBishop(HAND_B);
  list1[12] = f_hand_rook   + I2HandRook(HAND_W);
  list1[13] = e_hand_rook   + I2HandRook(HAND_B);

  if ( calc_difference( ptree, ply, turn, list0, list1, &score ) )
    {
      ehash_store( HASH_KEY, HAND_B, score );
      score                 = turn ? -score : score;
      ptree->save_eval[ply] = score;

      return score / FV_SCALE;
    }

  nlist = make_list( ptree, &score, list0, list1 );
  sq_bk = SQ_BKING;
  sq_wk = Inv( SQ_WKING );

  sum = 0;
  for ( i = 0; i < nlist; i++ )
    {
      k0 = list0[i];
      k1 = list1[i];
      for ( j = 0; j <= i; j++ )
	{
	  l0 = list0[j];
	  l1 = list1[j];
	  assert( k0 >= l0 && k1 >= l1 );
	  sum += PcPcOnSq( sq_bk, k0, l0 );
	  sum -= PcPcOnSq( sq_wk, k1, l1 );
	}
    }
  
  score += sum;
  score += MATERIAL * FV_SCALE;

#if defined(ENTERING_KING_BONUS)
  score += ENTERING_KING_score(ptree);
#endif

#if defined(INANIWA_SHIFT)
  score += inaniwa_score( ptree );
#endif
#ifdef ENTERING_KING
  score += ptree->save_eking_black[ply] - ptree->save_eking_black[ply - 1] - (ptree->save_eking_white[ply] - ptree->save_eking_white[ply - 1]);
#endif
  ehash_store( HASH_KEY, HAND_B, score );

  score = turn ? -score : score;

  ptree->save_eval[ply] = score;

  score /= FV_SCALE;

#if ! defined(MINIMUM)
  if ( abs(score) > score_max_eval )
    {
      out_warning( "A score at evaluate() is out of bounce." );
    }
#endif

  return score;

}


void CONV ehash_clear( void ) { memset( ehash_tbl, 0, sizeof(ehash_tbl) ); }


static int CONV ehash_probe( uint64_t current_key, unsigned int hand_b,
			     int * restrict pscore )
{
  uint64_t hash_word, hash_key;

  hash_word = ehash_tbl[ (unsigned int)current_key & EHASH_MASK ];

#if ! defined(__x86_64__)
  hash_word ^= hash_word << 32;
#endif

  current_key ^= (uint64_t)hand_b << 21;
  current_key &= ~(uint64_t)0x1fffffU;

  hash_key  = hash_word;
  hash_key &= ~(uint64_t)0x1fffffU;

  if ( hash_key != current_key ) { return 0; }

  *pscore = (int)( (unsigned int)hash_word & 0x1fffffU ) - 0x100000;

  return 1;
}


static void CONV ehash_store( uint64_t key, unsigned int hand_b, int score )
{
  uint64_t hash_word;

  hash_word  = key;
  hash_word ^= (uint64_t)hand_b << 21;
  hash_word &= ~(uint64_t)0x1fffffU;
  hash_word |= (uint64_t)( score + 0x100000 );

#if ! defined(__x86_64__)
  hash_word ^= hash_word << 32;
#endif

  ehash_tbl[ (unsigned int)key & EHASH_MASK ] = hash_word;
}


static int CONV
calc_difference( const tree_t * restrict ptree, int ply, int turn,
		 int list0[52], int list1[52], int * restrict pscore )
{
  bitboard_t bb;
  int nlist, diff, from, to, sq, pc;

#if defined(ENTERING_KING_BONUS)
  if (ENTERING_KING_flag) { return 0; }
#endif

#if defined(INANIWA_SHIFT)
  if ( inaniwa_flag ) { return 0; }
#endif

  if ( ptree->save_eval[ply-1] == INT_MAX ) { return 0; }
  if ( I2PieceMove(MOVE_LAST)  == king )    { return 0; }

  assert( MOVE_LAST != MOVE_PASS );

  nlist = 14;
  diff  = 0;
  from  = I2From(MOVE_LAST);
  to    = I2To(MOVE_LAST);

  BBOr( bb, BB_BOCCUPY, BB_WOCCUPY );
  Xor( SQ_BKING, bb );
  Xor( SQ_WKING, bb );
  Xor( to,       bb );
    
  while ( BBTest(bb) )
    {
      sq = FirstOne(bb);
      Xor( sq, bb );
      
      pc = BOARD[sq];
      list0[nlist  ] = aikpp[15+pc] + sq;
      list1[nlist++] = aikpp[15-pc] + Inv(sq);
    }

  pc = BOARD[to];
  list0[nlist  ] = aikpp[15+pc] + to;
  list1[nlist++] = aikpp[15-pc] + Inv(to);

  diff = doapc( ptree, pc, to, list0, list1, nlist );
  nlist -= 1;

  if ( from >= nsquare )
    {
      unsigned int hand;
      int hand_index;

      pc   = From2Drop(from);
      hand = turn ? HAND_B : HAND_W;

      switch ( pc )
	{
	case pawn:   hand_index = I2HandPawn(hand);   break;
	case lance:  hand_index = I2HandLance(hand);  break;
	case knight: hand_index = I2HandKnight(hand); break;
	case silver: hand_index = I2HandSilver(hand); break;
	case gold:   hand_index = I2HandGold(hand);   break;
	case bishop: hand_index = I2HandBishop(hand); break;
	default:     hand_index = I2HandRook(hand);   break;
	}

      diff += doacapt( ptree, pc, turn, hand_index, list0, list1, nlist );

      list0[ 2*(pc-1) + 1 - turn ] += 1;
      list1[ 2*(pc-1) + turn     ] += 1;
      hand_index                   += 1;

      diff -= doacapt( ptree, pc, turn, hand_index, list0, list1, nlist );
    }
  else {
    int pc_cap = UToCap(MOVE_LAST);
    if ( pc_cap )
      {
	unsigned int hand;
	int hand_index;

	pc     = pc_cap & ~promote;
	hand   = turn ? HAND_B : HAND_W;
	pc_cap = turn ? -pc_cap : pc_cap;
	diff  += turn ?  p_value_ex[15+pc_cap] * FV_SCALE
                      : -p_value_ex[15+pc_cap] * FV_SCALE;

	switch ( pc )
	  {
	  case pawn:   hand_index = I2HandPawn(hand);   break;
	  case lance:  hand_index = I2HandLance(hand);  break;
	  case knight: hand_index = I2HandKnight(hand); break;
	  case silver: hand_index = I2HandSilver(hand); break;
	  case gold:   hand_index = I2HandGold(hand);   break;
	  case bishop: hand_index = I2HandBishop(hand); break;
	  default:     hand_index = I2HandRook(hand);   break;
	  }

	diff += doacapt( ptree, pc, turn, hand_index, list0, list1, nlist );
	
	list0[ 2*(pc-1) + 1 - turn ] -= 1;
	list1[ 2*(pc-1) + turn     ] -= 1;
	hand_index                   -= 1;
	
	diff -= doacapt( ptree, pc, turn, hand_index, list0, list1, nlist );

	list0[nlist  ] = aikpp[15+pc_cap] + to;
	list1[nlist++] = aikpp[15-pc_cap] + Inv(to);

	diff -= doapc( ptree, pc_cap, to, list0, list1, nlist );
    }

    pc = I2PieceMove(MOVE_LAST);
    if ( I2IsPromote(MOVE_LAST) )
      {
	diff += ( turn ? p_value_pm[7+pc] : -p_value_pm[7+pc] ) * FV_SCALE;
      }
    
    pc = turn ? pc : -pc;

    list0[nlist  ] = aikpp[15+pc] + from;
    list1[nlist++] = aikpp[15-pc] + Inv(from);

    diff -= doapc( ptree, pc, from, list0, list1, nlist );
  
  }
  #ifdef ENTERING_KING
    diff += ptree->save_eking_black[ply] - ptree->save_eking_black[ply - 1] - (ptree->save_eking_white[ply] - ptree->save_eking_white[ply - 1]);
  #endif

  diff += turn ? ptree->save_eval[ply-1] : - ptree->save_eval[ply-1];

  *pscore = diff;

  return 1;
}


static int CONV
make_list( const tree_t * restrict ptree, int * restrict pscore,
	   int list0[52], int list1[52] )
{
  bitboard_t bb;
  int list2[34];
  int nlist, sq, n2, i, score, sq_bk0, sq_wk0, sq_bk1, sq_wk1;

  nlist  = 14;
  score  = 0;
  sq_bk0 = SQ_BKING;
  sq_wk0 = SQ_WKING;
  sq_bk1 = Inv(SQ_WKING);
  sq_wk1 = Inv(SQ_BKING);

  score += kkp[sq_bk0][sq_wk0][ kkp_hand_pawn   + I2HandPawn(HAND_B) ];
  score += kkp[sq_bk0][sq_wk0][ kkp_hand_lance  + I2HandLance(HAND_B) ];
  score += kkp[sq_bk0][sq_wk0][ kkp_hand_knight + I2HandKnight(HAND_B) ];
  score += kkp[sq_bk0][sq_wk0][ kkp_hand_silver + I2HandSilver(HAND_B) ];
  score += kkp[sq_bk0][sq_wk0][ kkp_hand_gold   + I2HandGold(HAND_B) ];
  score += kkp[sq_bk0][sq_wk0][ kkp_hand_bishop + I2HandBishop(HAND_B) ];
  score += kkp[sq_bk0][sq_wk0][ kkp_hand_rook   + I2HandRook(HAND_B) ];

  score -= kkp[sq_bk1][sq_wk1][ kkp_hand_pawn   + I2HandPawn(HAND_W) ];
  score -= kkp[sq_bk1][sq_wk1][ kkp_hand_lance  + I2HandLance(HAND_W) ];
  score -= kkp[sq_bk1][sq_wk1][ kkp_hand_knight + I2HandKnight(HAND_W) ];
  score -= kkp[sq_bk1][sq_wk1][ kkp_hand_silver + I2HandSilver(HAND_W) ];
  score -= kkp[sq_bk1][sq_wk1][ kkp_hand_gold   + I2HandGold(HAND_W) ];
  score -= kkp[sq_bk1][sq_wk1][ kkp_hand_bishop + I2HandBishop(HAND_W) ];
  score -= kkp[sq_bk1][sq_wk1][ kkp_hand_rook   + I2HandRook(HAND_W) ];

  n2 = 0;
  bb = BB_BPAWN;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_pawn + sq;
    list2[n2]    = e_pawn + Inv(sq);
    score += kkp[sq_bk0][sq_wk0][ kkp_pawn + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WPAWN;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_pawn + sq;
    list2[n2]    = f_pawn + Inv(sq);
    score -= kkp[sq_bk1][sq_wk1][ kkp_pawn + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }

  n2 = 0;
  bb = BB_BLANCE;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_lance + sq;
    list2[n2]    = e_lance + Inv(sq);
    score += kkp[sq_bk0][sq_wk0][ kkp_lance + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WLANCE;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_lance + sq;
    list2[n2]    = f_lance + Inv(sq);
    score -= kkp[sq_bk1][sq_wk1][ kkp_lance + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }


  n2 = 0;
  bb = BB_BKNIGHT;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_knight + sq;
    list2[n2]    = e_knight + Inv(sq);
    score += kkp[sq_bk0][sq_wk0][ kkp_knight + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WKNIGHT;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_knight + sq;
    list2[n2]    = f_knight + Inv(sq);
    score -= kkp[sq_bk1][sq_wk1][ kkp_knight + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }


  n2 = 0;
  bb = BB_BSILVER;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_silver + sq;
    list2[n2]    = e_silver + Inv(sq);
    score += kkp[sq_bk0][sq_wk0][ kkp_silver + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WSILVER;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_silver + sq;
    list2[n2]    = f_silver + Inv(sq);
    score -= kkp[sq_bk1][sq_wk1][ kkp_silver + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }


  n2 = 0;
  bb = BB_BTGOLD;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_gold + sq;
    list2[n2]    = e_gold + Inv(sq);
    score += kkp[sq_bk0][sq_wk0][ kkp_gold + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WTGOLD;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_gold + sq;
    list2[n2]    = f_gold + Inv(sq);
    score -= kkp[sq_bk1][sq_wk1][ kkp_gold + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }


  n2 = 0;
  bb = BB_BBISHOP;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_bishop + sq;
    list2[n2]    = e_bishop + Inv(sq);
    score += kkp[sq_bk0][sq_wk0][ kkp_bishop + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WBISHOP;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_bishop + sq;
    list2[n2]    = f_bishop + Inv(sq);
    score -= kkp[sq_bk1][sq_wk1][ kkp_bishop + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }


  n2 = 0;
  bb = BB_BHORSE;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_horse + sq;
    list2[n2]    = e_horse + Inv(sq);
    score += kkp[sq_bk0][sq_wk0][ kkp_horse + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WHORSE;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_horse + sq;
    list2[n2]    = f_horse + Inv(sq);
    score -= kkp[sq_bk1][sq_wk1][ kkp_horse + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }


  n2 = 0;
  bb = BB_BROOK;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_rook + sq;
    list2[n2]    = e_rook + Inv(sq);
    score += kkp[sq_bk0][sq_wk0][ kkp_rook + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WROOK;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_rook + sq;
    list2[n2]    = f_rook + Inv(sq);
    score -= kkp[sq_bk1][sq_wk1][ kkp_rook + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }


  n2 = 0;
  bb = BB_BDRAGON;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_dragon + sq;
    list2[n2]    = e_dragon + Inv(sq);
    score += kkp[sq_bk0][sq_wk0][ kkp_dragon + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WDRAGON;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_dragon + sq;
    list2[n2]    = f_dragon + Inv(sq);
    score -= kkp[sq_bk1][sq_wk1][ kkp_dragon + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }

  assert( nlist <= 52 );

  *pscore = score;
  return nlist;
}


static int CONV doapc( const tree_t * restrict ptree, int pc, int sq,
		       const int list0[52], const int list1[52], int nlist )
{
  int i, sum;
  int index_b = aikpp[15+pc] + sq;
  int index_w = aikpp[15-pc] + Inv(sq);
  int sq_bk   = SQ_BKING;
  int sq_wk   = Inv(SQ_WKING);
  
  sum = 0;
  for( i = 0; i < 14; i++ )
    {
      sum += PcPcOnSq( sq_bk, index_b, list0[i] );
      sum -= PcPcOnSq( sq_wk, index_w, list1[i] );
    }
  
  for( i = 14; i < nlist; i++ )
    {
      sum += PcPcOnSqAny( sq_bk, index_b, list0[i] );
      sum -= PcPcOnSqAny( sq_wk, index_w, list1[i] );
    }
  
  if ( pc > 0 )
    {
      sq_bk  = SQ_BKING;
      sq_wk  = SQ_WKING;
      sum   += kkp[sq_bk][sq_wk][ aikkp[pc] + sq ];
    }
  else {
    sq_bk  = Inv(SQ_WKING);
    sq_wk  = Inv(SQ_BKING);
    sum   -= kkp[sq_bk][sq_wk][ aikkp[-pc] + Inv(sq) ];
  }
  
  return sum;
}


static int CONV
doacapt( const tree_t * restrict ptree, int pc, int turn, int hand_index,
	 const int list0[52], const int list1[52], int nlist )
{
  int i, sum, sq_bk, sq_wk, index_b, index_w;
  
  index_b = 2*(pc-1) + 1 - turn;
  index_w = 2*(pc-1) + turn;
  sq_bk   = SQ_BKING;
  sq_wk   = Inv(SQ_WKING);
  
  sum = 0;
  for( i = 14; i < nlist; i++ )
    {
      sum += PcPcOnSq( sq_bk, list0[i], list0[index_b] );
      sum -= PcPcOnSq( sq_wk, list1[i], list1[index_w] );
    }

  for( i = 0; i <= 2*(pc-1); i++ )
    {
      sum += PcPcOnSq( sq_bk, list0[index_b], list0[i] );
      sum -= PcPcOnSq( sq_wk, list1[index_w], list1[i] );
    }

  for( i += 1; i < 14; i++ )
    {
      sum += PcPcOnSq( sq_bk, list0[i], list0[index_b] );
      sum -= PcPcOnSq( sq_wk, list1[i], list1[index_w] );
    }

  if ( turn )
    {
      sum += PcPcOnSq( sq_bk, list0[index_w], list0[index_b] );
      sum -= PcPcOnSq( sq_wk, list1[index_w], list1[index_w] );
      sq_bk = SQ_BKING;
      sq_wk = SQ_WKING;
      sum  += kkp[sq_bk][sq_wk][ aikkp_hand[pc] + hand_index ];
    }
  else {
    sum += PcPcOnSq( sq_bk, list0[index_b], list0[index_b] );
    sum -= PcPcOnSq( sq_wk, list1[index_b], list1[index_w] );
    sq_bk = Inv(SQ_WKING);
    sq_wk = Inv(SQ_BKING);
    sum  -= kkp[sq_bk][sq_wk][ aikkp_hand[pc] + hand_index ];
  }
  
  return sum;
}

#if defined(ENTERING_KING_BONUS)
static int
ENTERING_KING_score(const tree_t * restrict ptree)
{
	int score;

	if (!ENTERING_KING_flag) { return 0; }

	score = 0;
	if (ENTERING_KING_flag == 1) {
		//後手番の玉の位置が入玉フラグ位置の時、先手玉位置に加点
		if (BOARD[A9] == king) { score += 1000 * FV_SCALE; } //1000 900 900 850 800 850 900 900 1000
		if (BOARD[B9] == king) { score +=  900 * FV_SCALE; } // 850 750 750 700 650 700 750 750  850
		if (BOARD[C9] == king) { score +=  900 * FV_SCALE; } // 680 630 630 580 530 580 630 630  680
		if (BOARD[D9] == king) { score +=  850 * FV_SCALE; } // 500 500 500 450 400 450 500 500  500
		if (BOARD[E9] == king) { score +=  800 * FV_SCALE; } // 330 330 330 280 230 280 330 330  330
		if (BOARD[F9] == king) { score +=  850 * FV_SCALE; } // 180 180 180 130  90 130 180 180  180
		if (BOARD[G9] == king) { score +=  900 * FV_SCALE; } //  90  90  90   0   0   0  90  90   90
		if (BOARD[H9] == king) { score +=  900 * FV_SCALE; } //   0   0   0   0   0   0   0   0    0
		if (BOARD[I9] == king) { score += 1000 * FV_SCALE; } //-200   0   0-200-225-200   0   0 -200

		if (BOARD[A8] == king) { score += 850 * FV_SCALE; } // 850 750 750 700 650 700 750 750  850
		if (BOARD[B8] == king) { score += 750 * FV_SCALE; }
		if (BOARD[C8] == king) { score += 750 * FV_SCALE; }
		if (BOARD[D8] == king) { score += 700 * FV_SCALE; }
		if (BOARD[E8] == king) { score += 650 * FV_SCALE; }
		if (BOARD[F8] == king) { score += 700 * FV_SCALE; }
		if (BOARD[G8] == king) { score += 750 * FV_SCALE; }
		if (BOARD[H8] == king) { score += 750 * FV_SCALE; }
		if (BOARD[I8] == king) { score += 850 * FV_SCALE; }

		if (BOARD[A7] == king) { score += 680 * FV_SCALE; } // 680 630 630 580 530 580 630 630  680
		if (BOARD[B7] == king) { score += 630 * FV_SCALE; }
		if (BOARD[C7] == king) { score += 630 * FV_SCALE; }
		if (BOARD[D7] == king) { score += 580 * FV_SCALE; }
		if (BOARD[E7] == king) { score += 530 * FV_SCALE; }
		if (BOARD[F7] == king) { score += 580 * FV_SCALE; }
		if (BOARD[G7] == king) { score += 630 * FV_SCALE; }
		if (BOARD[H7] == king) { score += 630 * FV_SCALE; }
		if (BOARD[I7] == king) { score += 680 * FV_SCALE; }

		if (BOARD[A6] == king) { score += 500 * FV_SCALE; } // 500 500 500 450 400 450 500 500  500
		if (BOARD[B6] == king) { score += 500 * FV_SCALE; }
		if (BOARD[C6] == king) { score += 500 * FV_SCALE; }
		if (BOARD[D6] == king) { score += 450 * FV_SCALE; }
		if (BOARD[E6] == king) { score += 400 * FV_SCALE; }
		if (BOARD[F6] == king) { score += 450 * FV_SCALE; }
		if (BOARD[G6] == king) { score += 500 * FV_SCALE; }
		if (BOARD[H6] == king) { score += 500 * FV_SCALE; }
		if (BOARD[I6] == king) { score += 500 * FV_SCALE; }

		if (BOARD[A5] == king) { score += 330 * FV_SCALE; } // 330 330 330 280 230 280 330 330  330
		if (BOARD[B5] == king) { score += 330 * FV_SCALE; }
		if (BOARD[C5] == king) { score += 330 * FV_SCALE; }
		if (BOARD[D5] == king) { score += 280 * FV_SCALE; }
		if (BOARD[E5] == king) { score += 230 * FV_SCALE; }
		if (BOARD[F5] == king) { score += 280 * FV_SCALE; }
		if (BOARD[G5] == king) { score += 330 * FV_SCALE; }
		if (BOARD[H5] == king) { score += 330 * FV_SCALE; }
		if (BOARD[I5] == king) { score += 330 * FV_SCALE; }

		if (BOARD[A4] == king) { score += 180 * FV_SCALE; } // 180 180 180 130  90 130 180 180  180
		if (BOARD[B4] == king) { score += 180 * FV_SCALE; }
		if (BOARD[C4] == king) { score += 180 * FV_SCALE; }
		if (BOARD[D4] == king) { score += 130 * FV_SCALE; }
		if (BOARD[E4] == king) { score +=  90 * FV_SCALE; }
		if (BOARD[F4] == king) { score += 130 * FV_SCALE; }
		if (BOARD[G4] == king) { score += 180 * FV_SCALE; }
		if (BOARD[H4] == king) { score += 180 * FV_SCALE; }
		if (BOARD[I4] == king) { score += 180 * FV_SCALE; }

		if (BOARD[A3] == king) { score += 90 * FV_SCALE; } //  90   90   90    0    0    0   90   90   90
		if (BOARD[B3] == king) { score += 90 * FV_SCALE; }
		if (BOARD[C3] == king) { score += 90 * FV_SCALE; }
		if (BOARD[D3] == king) { score += 0 * FV_SCALE; }
		if (BOARD[E3] == king) { score += 0 * FV_SCALE; }
		if (BOARD[F3] == king) { score += 0 * FV_SCALE; }
		if (BOARD[G3] == king) { score += 90 * FV_SCALE; }
		if (BOARD[H3] == king) { score += 90 * FV_SCALE; }
		if (BOARD[I3] == king) { score += 90 * FV_SCALE; }

		if (BOARD[A1] == king) { score -= 200 * FV_SCALE; } //-200    0    0 -200 -225 -200    0    0 -200
		if (BOARD[B1] == king) { score -= 0 * FV_SCALE; }
		if (BOARD[C1] == king) { score -= 0 * FV_SCALE; }
		if (BOARD[D1] == king) { score -= 200 * FV_SCALE; }
		if (BOARD[E1] == king) { score -= 225 * FV_SCALE; }
		if (BOARD[F1] == king) { score -= 200 * FV_SCALE; }
		if (BOARD[G1] == king) { score -= 0 * FV_SCALE; }
		if (BOARD[H1] == king) { score -= 0 * FV_SCALE; }
		if (BOARD[I1] == king) { score -= 200 * FV_SCALE; }
		//後手番の玉の位置が入玉フラグ位置の時、後手玉位置に加点
		if (BOARD[A1] == -king) { score -= 1000 * FV_SCALE; } //1000 900 900 850 800 850 900 900 1000
		if (BOARD[B1] == -king) { score -=  900 * FV_SCALE; }
		if (BOARD[C1] == -king) { score -=  900 * FV_SCALE; }
		if (BOARD[D1] == -king) { score -=  850 * FV_SCALE; }
		if (BOARD[E1] == -king) { score -=  800 * FV_SCALE; }
		if (BOARD[F1] == -king) { score -=  850 * FV_SCALE; }
		if (BOARD[G1] == -king) { score -=  900 * FV_SCALE; }
		if (BOARD[H1] == -king) { score -=  900 * FV_SCALE; }
		if (BOARD[I1] == -king) { score -= 1000 * FV_SCALE; }

		if (BOARD[A2] == -king) { score -= 850 * FV_SCALE; } // 850 750 750 700 650 700 750 750  850
		if (BOARD[B2] == -king) { score -= 750 * FV_SCALE; }
		if (BOARD[C2] == -king) { score -= 750 * FV_SCALE; }
		if (BOARD[D2] == -king) { score -= 700 * FV_SCALE; }
		if (BOARD[E2] == -king) { score -= 650 * FV_SCALE; }
		if (BOARD[F2] == -king) { score -= 700 * FV_SCALE; }
		if (BOARD[G2] == -king) { score -= 750 * FV_SCALE; }
		if (BOARD[H2] == -king) { score -= 750 * FV_SCALE; }
		if (BOARD[I2] == -king) { score -= 850 * FV_SCALE; }

		if (BOARD[A3] == -king) { score -= 680 * FV_SCALE; } // 680 630 630 580 530 580 630 630  680
		if (BOARD[B3] == -king) { score -= 630 * FV_SCALE; }
		if (BOARD[C3] == -king) { score -= 630 * FV_SCALE; }
		if (BOARD[D3] == -king) { score -= 580 * FV_SCALE; }
		if (BOARD[E3] == -king) { score -= 530 * FV_SCALE; }
		if (BOARD[F3] == -king) { score -= 580 * FV_SCALE; }
		if (BOARD[G3] == -king) { score -= 630 * FV_SCALE; }
		if (BOARD[H3] == -king) { score -= 630 * FV_SCALE; }
		if (BOARD[I3] == -king) { score -= 680 * FV_SCALE; }

		if (BOARD[A4] == -king) { score -= 500 * FV_SCALE; } // 500 500 500 450 400 450 500 500  500
		if (BOARD[B4] == -king) { score -= 500 * FV_SCALE; }
		if (BOARD[C4] == -king) { score -= 500 * FV_SCALE; }
		if (BOARD[D4] == -king) { score -= 450 * FV_SCALE; }
		if (BOARD[E4] == -king) { score -= 400 * FV_SCALE; }
		if (BOARD[F4] == -king) { score -= 450 * FV_SCALE; }
		if (BOARD[G4] == -king) { score -= 500 * FV_SCALE; }
		if (BOARD[H4] == -king) { score -= 500 * FV_SCALE; }
		if (BOARD[I4] == -king) { score -= 500 * FV_SCALE; }

		if (BOARD[A5] == -king) { score -= 330 * FV_SCALE; } // 330 330 330 280 230 280 330 330  330
		if (BOARD[B5] == -king) { score -= 330 * FV_SCALE; }
		if (BOARD[C5] == -king) { score -= 330 * FV_SCALE; }
		if (BOARD[D5] == -king) { score -= 280 * FV_SCALE; }
		if (BOARD[E5] == -king) { score -= 230 * FV_SCALE; }
		if (BOARD[F5] == -king) { score -= 280 * FV_SCALE; }
		if (BOARD[G5] == -king) { score -= 330 * FV_SCALE; }
		if (BOARD[H5] == -king) { score -= 330 * FV_SCALE; }
		if (BOARD[I5] == -king) { score -= 330 * FV_SCALE; }

		if (BOARD[A6] == -king) { score -= 180 * FV_SCALE; } // 180 180 180 130  90 130 180 180  180
		if (BOARD[B6] == -king) { score -= 180 * FV_SCALE; }
		if (BOARD[C6] == -king) { score -= 180 * FV_SCALE; }
		if (BOARD[D6] == -king) { score -= 130 * FV_SCALE; }
		if (BOARD[E6] == -king) { score -=  90 * FV_SCALE; }
		if (BOARD[F6] == -king) { score -= 130 * FV_SCALE; }
		if (BOARD[G6] == -king) { score -= 180 * FV_SCALE; }
		if (BOARD[H6] == -king) { score -= 180 * FV_SCALE; }
		if (BOARD[I6] == -king) { score -= 180 * FV_SCALE; }

		if (BOARD[A7] == -king) { score -= 90 * FV_SCALE; } //  90   90   90    0    0    0   90   90   90
		if (BOARD[B7] == -king) { score -= 90 * FV_SCALE; }
		if (BOARD[C7] == -king) { score -= 90 * FV_SCALE; }
		if (BOARD[D7] == -king) { score -= 0 * FV_SCALE; }
		if (BOARD[E7] == -king) { score -= 0 * FV_SCALE; }
		if (BOARD[F7] == -king) { score -= 0 * FV_SCALE; }
		if (BOARD[G7] == -king) { score -= 90 * FV_SCALE; }
		if (BOARD[H7] == -king) { score -= 90 * FV_SCALE; }
		if (BOARD[I7] == -king) { score -= 90 * FV_SCALE; }

		if (BOARD[A9] == -king) { score += 200 * FV_SCALE; } //-200    0    0 -200 -225 -200    0    0 -200
		if (BOARD[B9] == -king) { score += 0 * FV_SCALE; }
		if (BOARD[C9] == -king) { score += 0 * FV_SCALE; }
		if (BOARD[D9] == -king) { score += 200 * FV_SCALE; }
		if (BOARD[E9] == -king) { score += 225 * FV_SCALE; }
		if (BOARD[F9] == -king) { score += 200 * FV_SCALE; }
		if (BOARD[G9] == -king) { score += 0 * FV_SCALE; }
		if (BOARD[H9] == -king) { score += 0 * FV_SCALE; }
		if (BOARD[I9] == -king) { score += 200 * FV_SCALE; }

 }
	else {
		//先手番の玉の位置が入玉フラグ位置の時、後手玉位置に加点
		if (BOARD[A1] == -king) { score -= 1000 * FV_SCALE; } //1000 900 900 850 800 850 900 900 1000
		if (BOARD[B1] == -king) { score -=  900 * FV_SCALE; }
		if (BOARD[C1] == -king) { score -=  900 * FV_SCALE; }
		if (BOARD[D1] == -king) { score -=  850 * FV_SCALE; }
		if (BOARD[E1] == -king) { score -=  800 * FV_SCALE; }
		if (BOARD[F1] == -king) { score -=  850 * FV_SCALE; }
		if (BOARD[G1] == -king) { score -=  900 * FV_SCALE; }
		if (BOARD[H1] == -king) { score -=  900 * FV_SCALE; }
		if (BOARD[I1] == -king) { score -= 1000 * FV_SCALE; }

		if (BOARD[A2] == -king) { score -= 850 * FV_SCALE; } // 850 750 750 700 650 700 750 750  850
		if (BOARD[B2] == -king) { score -= 750 * FV_SCALE; }
		if (BOARD[C2] == -king) { score -= 750 * FV_SCALE; }
		if (BOARD[D2] == -king) { score -= 700 * FV_SCALE; }
		if (BOARD[E2] == -king) { score -= 650 * FV_SCALE; }
		if (BOARD[F2] == -king) { score -= 700 * FV_SCALE; }
		if (BOARD[G2] == -king) { score -= 750 * FV_SCALE; }
		if (BOARD[H2] == -king) { score -= 750 * FV_SCALE; }
		if (BOARD[I2] == -king) { score -= 850 * FV_SCALE; }

		if (BOARD[A3] == -king) { score -= 680 * FV_SCALE; } // 680 630 630 580 530 580 630 630  680
		if (BOARD[B3] == -king) { score -= 630 * FV_SCALE; }
		if (BOARD[C3] == -king) { score -= 630 * FV_SCALE; }
		if (BOARD[D3] == -king) { score -= 580 * FV_SCALE; }
		if (BOARD[E3] == -king) { score -= 530 * FV_SCALE; }
		if (BOARD[F3] == -king) { score -= 580 * FV_SCALE; }
		if (BOARD[G3] == -king) { score -= 630 * FV_SCALE; }
		if (BOARD[H3] == -king) { score -= 630 * FV_SCALE; }
		if (BOARD[I3] == -king) { score -= 680 * FV_SCALE; }

		if (BOARD[A4] == -king) { score -= 500 * FV_SCALE; } // 500 500 500 450 400 450 500 500  500
		if (BOARD[B4] == -king) { score -= 500 * FV_SCALE; }
		if (BOARD[C4] == -king) { score -= 500 * FV_SCALE; }
		if (BOARD[D4] == -king) { score -= 450 * FV_SCALE; }
		if (BOARD[E4] == -king) { score -= 400 * FV_SCALE; }
		if (BOARD[F4] == -king) { score -= 450 * FV_SCALE; }
		if (BOARD[G4] == -king) { score -= 500 * FV_SCALE; }
		if (BOARD[H4] == -king) { score -= 500 * FV_SCALE; }
		if (BOARD[I4] == -king) { score -= 500 * FV_SCALE; }

		if (BOARD[A5] == -king) { score -= 330 * FV_SCALE; } // 330 330 330 280 230 280 330 330  330
		if (BOARD[B5] == -king) { score -= 330 * FV_SCALE; }
		if (BOARD[C5] == -king) { score -= 330 * FV_SCALE; }
		if (BOARD[D5] == -king) { score -= 280 * FV_SCALE; }
		if (BOARD[E5] == -king) { score -= 230 * FV_SCALE; }
		if (BOARD[F5] == -king) { score -= 280 * FV_SCALE; }
		if (BOARD[G5] == -king) { score -= 330 * FV_SCALE; }
		if (BOARD[H5] == -king) { score -= 330 * FV_SCALE; }
		if (BOARD[I5] == -king) { score -= 330 * FV_SCALE; }

		if (BOARD[A6] == -king) { score -= 180 * FV_SCALE; } // 180 180 180 130  90 130 180 180  180
		if (BOARD[B6] == -king) { score -= 180 * FV_SCALE; }
		if (BOARD[C6] == -king) { score -= 180 * FV_SCALE; }
		if (BOARD[D6] == -king) { score -= 130 * FV_SCALE; }
		if (BOARD[E6] == -king) { score -=  90 * FV_SCALE; }
		if (BOARD[F6] == -king) { score -= 130 * FV_SCALE; }
		if (BOARD[G6] == -king) { score -= 180 * FV_SCALE; }
		if (BOARD[H6] == -king) { score -= 180 * FV_SCALE; }
		if (BOARD[I6] == -king) { score -= 180 * FV_SCALE; }

		if (BOARD[A7] == -king) { score -= 90 * FV_SCALE; } //  90   90   90    0    0    0   90   90   90
		if (BOARD[B7] == -king) { score -= 90 * FV_SCALE; }
		if (BOARD[C7] == -king) { score -= 90 * FV_SCALE; }
		if (BOARD[D7] == -king) { score -= 0 * FV_SCALE; }
		if (BOARD[E7] == -king) { score -= 0 * FV_SCALE; }
		if (BOARD[F7] == -king) { score -= 0 * FV_SCALE; }
		if (BOARD[G7] == -king) { score -= 90 * FV_SCALE; }
		if (BOARD[H7] == -king) { score -= 90 * FV_SCALE; }
		if (BOARD[I7] == -king) { score -= 90 * FV_SCALE; }

		if (BOARD[A9] == -king) { score += 200 * FV_SCALE; } //-200    0    0 -200 -225 -200    0    0 -200
		if (BOARD[B9] == -king) { score += 0 * FV_SCALE; }
		if (BOARD[C9] == -king) { score += 0 * FV_SCALE; }
		if (BOARD[D9] == -king) { score += 200 * FV_SCALE; }
		if (BOARD[E9] == -king) { score += 225 * FV_SCALE; }
		if (BOARD[F9] == -king) { score += 200 * FV_SCALE; }
		if (BOARD[G9] == -king) { score += 0 * FV_SCALE; }
		if (BOARD[H9] == -king) { score += 0 * FV_SCALE; }
		if (BOARD[I9] == -king) { score += 200 * FV_SCALE; }
		//先手番の玉の位置が入玉フラグ位置の時、先手玉位置に加点
		if (BOARD[A9] == king) { score += 1000 * FV_SCALE; } //1000 900 900 850 800 850 900 900 1000
		if (BOARD[B9] == king) { score +=  900 * FV_SCALE; }
		if (BOARD[C9] == king) { score +=  900 * FV_SCALE; }
		if (BOARD[D9] == king) { score +=  850 * FV_SCALE; }
		if (BOARD[E9] == king) { score +=  800 * FV_SCALE; }
		if (BOARD[F9] == king) { score +=  850 * FV_SCALE; }
		if (BOARD[G9] == king) { score +=  900 * FV_SCALE; }
		if (BOARD[H9] == king) { score +=  900 * FV_SCALE; }
		if (BOARD[I9] == king) { score += 1000 * FV_SCALE; }

		if (BOARD[A8] == king) { score += 850 * FV_SCALE; } // 850 750 750 700 650 700 750 750  850
		if (BOARD[B8] == king) { score += 750 * FV_SCALE; }
		if (BOARD[C8] == king) { score += 750 * FV_SCALE; }
		if (BOARD[D8] == king) { score += 700 * FV_SCALE; }
		if (BOARD[E8] == king) { score += 650 * FV_SCALE; }
		if (BOARD[F8] == king) { score += 700 * FV_SCALE; }
		if (BOARD[G8] == king) { score += 750 * FV_SCALE; }
		if (BOARD[H8] == king) { score += 750 * FV_SCALE; }
		if (BOARD[I8] == king) { score += 850 * FV_SCALE; }

		if (BOARD[A7] == king) { score += 680 * FV_SCALE; } // 680 630 630 580 530 580 630 630  680
		if (BOARD[B7] == king) { score += 630 * FV_SCALE; }
		if (BOARD[C7] == king) { score += 630 * FV_SCALE; }
		if (BOARD[D7] == king) { score += 580 * FV_SCALE; }
		if (BOARD[E7] == king) { score += 530 * FV_SCALE; }
		if (BOARD[F7] == king) { score += 580 * FV_SCALE; }
		if (BOARD[G7] == king) { score += 630 * FV_SCALE; }
		if (BOARD[H7] == king) { score += 630 * FV_SCALE; }
		if (BOARD[I7] == king) { score += 680 * FV_SCALE; }

		if (BOARD[A6] == king) { score += 500 * FV_SCALE; } // 500 500 500 450 400 450 500 500  500
		if (BOARD[B6] == king) { score += 500 * FV_SCALE; }
		if (BOARD[C6] == king) { score += 500 * FV_SCALE; }
		if (BOARD[D6] == king) { score += 450 * FV_SCALE; }
		if (BOARD[E6] == king) { score += 400 * FV_SCALE; }
		if (BOARD[F6] == king) { score += 450 * FV_SCALE; }
		if (BOARD[G6] == king) { score += 500 * FV_SCALE; }
		if (BOARD[H6] == king) { score += 500 * FV_SCALE; }
		if (BOARD[I6] == king) { score += 500 * FV_SCALE; }

		if (BOARD[A5] == king) { score += 330 * FV_SCALE; } // 330 330 330 280 230 280 330 330  330
		if (BOARD[B5] == king) { score += 330 * FV_SCALE; }
		if (BOARD[C5] == king) { score += 330 * FV_SCALE; }
		if (BOARD[D5] == king) { score += 280 * FV_SCALE; }
		if (BOARD[E5] == king) { score += 230 * FV_SCALE; }
		if (BOARD[F5] == king) { score += 280 * FV_SCALE; }
		if (BOARD[G5] == king) { score += 330 * FV_SCALE; }
		if (BOARD[H5] == king) { score += 330 * FV_SCALE; }
		if (BOARD[I5] == king) { score += 330 * FV_SCALE; }

		if (BOARD[A4] == king) { score += 180 * FV_SCALE; } // 180 180 180 130  90 130 180 180  180
		if (BOARD[B4] == king) { score += 180 * FV_SCALE; }
		if (BOARD[C4] == king) { score += 180 * FV_SCALE; }
		if (BOARD[D4] == king) { score += 130 * FV_SCALE; }
		if (BOARD[E4] == king) { score +=  90 * FV_SCALE; }
		if (BOARD[F4] == king) { score += 130 * FV_SCALE; }
		if (BOARD[G4] == king) { score += 180 * FV_SCALE; }
		if (BOARD[H4] == king) { score += 180 * FV_SCALE; }
		if (BOARD[I4] == king) { score += 180 * FV_SCALE; }

		if (BOARD[A3] == king) { score += 90 * FV_SCALE; } //  90   90   90    0    0    0   90   90   90
		if (BOARD[B3] == king) { score += 90 * FV_SCALE; }
		if (BOARD[C3] == king) { score += 90 * FV_SCALE; }
		if (BOARD[D3] == king) { score += 0 * FV_SCALE; }
		if (BOARD[E3] == king) { score += 0 * FV_SCALE; }
		if (BOARD[F3] == king) { score += 0 * FV_SCALE; }
		if (BOARD[G3] == king) { score += 90 * FV_SCALE; }
		if (BOARD[H3] == king) { score += 90 * FV_SCALE; }
		if (BOARD[I3] == king) { score += 90 * FV_SCALE; }

		if (BOARD[A1] == king) { score -= 200 * FV_SCALE; } //-200    0    0 -200 -225 -200    0    0 -200
		if (BOARD[B1] == king) { score -= 0 * FV_SCALE; }
		if (BOARD[C1] == king) { score -= 0 * FV_SCALE; }
		if (BOARD[D1] == king) { score -= 200 * FV_SCALE; }
		if (BOARD[E1] == king) { score -= 225 * FV_SCALE; }
		if (BOARD[F1] == king) { score -= 200 * FV_SCALE; }
		if (BOARD[G1] == king) { score -= 0 * FV_SCALE; }
		if (BOARD[H1] == king) { score -= 0 * FV_SCALE; }
		if (BOARD[I1] == king) { score -= 200 * FV_SCALE; }
	}

	return score;
}
#endif

#if defined(INANIWA_SHIFT)
static int
inaniwa_score( const tree_t * restrict ptree )
{
  int score;

  if ( ! inaniwa_flag ) { return 0; }

  score = 0;
  if ( inaniwa_flag == 2 ) {
	  //先手番の歩が突かれていない時、後手番の桂馬初期配置に減点
    if ( BOARD[B9] == -knight ) { score += 700 * FV_SCALE; }
    if ( BOARD[H9] == -knight ) { score += 700 * FV_SCALE; }
    
    if ( BOARD[A7] == -knight ) { score += 700 * FV_SCALE; }
    if ( BOARD[C7] == -knight ) { score += 400 * FV_SCALE; }
    if ( BOARD[G7] == -knight ) { score += 400 * FV_SCALE; }
    if ( BOARD[I7] == -knight ) { score += 700 * FV_SCALE; }
    
    if ( BOARD[B5] == -knight ) { score += 700 * FV_SCALE; }
    if ( BOARD[D5] == -knight ) { score += 100 * FV_SCALE; }
    if ( BOARD[F5] == -knight ) { score += 100 * FV_SCALE; }
    if ( BOARD[H5] == -knight ) { score += 700 * FV_SCALE; }

    if ( BOARD[E3] ==  pawn )   { score += 200 * FV_SCALE; }
    if ( BOARD[E4] ==  pawn )   { score += 200 * FV_SCALE; }
    if ( BOARD[E5] ==  pawn )   { score += 200 * FV_SCALE; }

  } else {
	  //後手番の歩が突かれていない時、先手番の桂馬初期配置に減点
    if ( BOARD[B1] ==  knight ) { score -= 700 * FV_SCALE; }
    if ( BOARD[H1] ==  knight ) { score -= 700 * FV_SCALE; }
    
    if ( BOARD[A3] ==  knight ) { score -= 700 * FV_SCALE; }
    if ( BOARD[C3] ==  knight ) { score -= 400 * FV_SCALE; }
    if ( BOARD[G3] ==  knight ) { score -= 400 * FV_SCALE; }
    if ( BOARD[I3] ==  knight ) { score -= 700 * FV_SCALE; }
    
    if ( BOARD[B5] ==  knight ) { score -= 700 * FV_SCALE; }
    if ( BOARD[D5] ==  knight ) { score -= 100 * FV_SCALE; }
    if ( BOARD[F5] ==  knight ) { score -= 100 * FV_SCALE; }
    if ( BOARD[H5] ==  knight ) { score -= 700 * FV_SCALE; }

    if ( BOARD[E7] == -pawn )   { score -= 200 * FV_SCALE; }
    if ( BOARD[E6] == -pawn )   { score -= 200 * FV_SCALE; }
    if ( BOARD[E5] == -pawn )   { score -= 200 * FV_SCALE; }
  }

  return score;
}
#endif
#if defined(ENTERING_KING)
int CONV eking_value_black_position(tree_t * restrict ptree, unsigned char bking, bitboard_t *visited) {
   if (airank[bking] <= rank3)return 0;
   if (BBContract(*visited, abb_mask[bking])) return -1;
   BBOr(*visited, *visited, abb_mask[bking]);
   if (aifile[bking] != file1) {
      int p_lu = bking - 10;
      if (BOARD[p_lu] == empty && !is_black_attacked(ptree, p_lu)) {
        int ret_lu = eking_value_black_position(ptree, p_lu, visited);
      }
     }
     {
       int p_u = bking - 9;
       if (BOARD[p_u] == empty && !is_black_attacked(ptree, p_u)) {
          int ret_u = eking_value_black_position(ptree, p_u, visited);
          if (0 <= ret_u) return ret_u + 1;
        }
        }
     if (aifile[bking] != file9) {
     int p_ru = bking - 8;
      if (BOARD[p_ru] == empty && !is_black_attacked(ptree, p_ru)) {
          int ret_ru = eking_value_black_position(ptree, p_ru, visited);
          if (0 <= ret_ru) return ret_ru + 1;
        }
     }
    return -1;
}
int CONV eking_value_white_position(tree_t * restrict ptree, unsigned char wking, bitboard_t *visited) {
   if (airank[wking] >= rank7)return 0;
   if (BBContract(*visited, abb_mask[wking])) return -1;
   BBOr(*visited, *visited, abb_mask[wking]);
   if (aifile[wking] != file1) {
      int p_ld = wking + 8;
      if (BOARD[p_ld] == empty && !is_white_attacked(ptree, p_ld)) {
          int ret_ld = eking_value_white_position(ptree, p_ld, visited);
          if (0 <= ret_ld) return ret_ld + 1;
        }
      }
      {
        int p_d = wking + 9;
        if (BOARD[p_d] == empty && !is_white_attacked(ptree, p_d)) {
           int ret_d = eking_value_white_position(ptree, p_d, visited);
           if (0 <= ret_d) return ret_d + 1;
        }
        }
   if (aifile[wking] != file9) {
       int p_rd = wking + 10;
       if (BOARD[p_rd] == empty && !is_white_attacked(ptree, p_rd)) {
           int ret_rd = eking_value_white_position(ptree, p_rd, visited);
           if (0 <= ret_rd) return ret_rd + 1;
        }
      }
    return -1;
}
int CONV eking_value_black(tree_t * restrict ptree) {
    bitboard_t visited;
    BBIni(visited);
    return eking_value_black_position(ptree, SQ_BKING, &visited);
}
int CONV eking_value_white(tree_t * restrict ptree) {
    bitboard_t visited;
    BBIni(visited);
    return eking_value_white_position(ptree, SQ_WKING, &visited);
}
int CONV eking_value(tree_t * restrict ptree, int turn)
 {
   return turn ? eking_value_white(ptree) : eking_value_black(ptree);
    }
#endif