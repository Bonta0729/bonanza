#include <limits.h>
#include <assert.h>
#if ! defined(_WIN32)
#  include <sys/time.h>
#  include <time.h>
#endif
#include "shogi.h"


void CONV
set_search_limit_time( int turn )
/*
  [inputs]
  global variables:
    sec_limit       time limit of a side for an entire game in seconds
    sec_limit_up    time limit for a move when the ordinary time has run out.
                    the time is known as 'byo-yomi'.
    sec_b_total     seconds spent by black 
    sec_w_total     seconds spent by white
    time_response
    ( game_status & flag_puzzling )        time-control for puzzling-search
    ( game_status & flag_pondering )       time-control for pondering-search
    ( game_status & flag_time_extendable ) bonanza isn't punctual to the time
  
  argument:
    turn            a side to move
  
  [outputs]
  global variables:
    time_limit      tentative deadline for searching in millisecond
    time_max_limit  maximum allowed time to search in millisecond
  
  [working area]
  local variables:
    sec_elapsed     second spent by the side to move
    sec_left        time left for the side in seconds
    u0              tentative deadline for searching in second
    u1              maximum allowed time to search in second
*/
{
  unsigned int u0, u1;

  /* no time-control */
  if ( sec_limit_up == UINT_MAX || ( game_status & flag_pondering ) )
    {
      time_max_limit = time_limit = UINT_MAX;
      return;
    }

#if defined(USI)
  if ( usi_mode != usi_off && usi_byoyomi )
    {
      time_max_limit = time_limit = usi_byoyomi;
      Out( "- time ctrl: %u -- %u\n", time_limit, time_max_limit );
      return;
    }
 /* else if (usi_mode != usi_off && usi_inc)
  {
	  time_max_limit = time_limit = usi_inc;
	  Out("- time ctrl: %u -- %u\n", time_limit, time_max_limit);
	  return;
  }*/
#endif

  /* not punctual to the time */
  if ( ! sec_limit && ( game_status & flag_time_extendable ) )
    {
      u0 = sec_limit_up;
      u1 = sec_limit_up * 5U;
    }
  /* have byo-yomi */
  else if (sec_limit_up)
  {
	  unsigned int umax, umin, sec_elapsed, sec_left/*, wtime, btime, winc, binc*/;
	  //�����
	  sec_elapsed = turn ? sec_w_total : sec_b_total;
	  //�c�莞��=�������Ԃ�����Ԉȏ�Ȃ�A��������-����ԁB
	  sec_left = (sec_elapsed <= sec_limit) ? sec_limit - sec_elapsed : 0;// turn ? wtime : btime;
	  u0 = (sec_left + (TC_NMOVE / 2U)) / TC_NMOVE;

	  /*
	  t = 2s is not beneficial since 2.8s are almost the same as 1.8s.
	  So that, we rather want to use up the ordinary time.
	  */
	  if (u0 == 2U) { u0 = 3U; }

	  /* 'byo-yomi' is so long that the ordinary time-limit is negligible. */
	  if (sec_left > sec_limit && sec_elapsed <sec_limit) {
		  // �c�莞�Ԃ��������Ԃ�葽���B(�t�B�b�V���[���[�h�p) ��Ղ𔲂�������ɑ��߂Ɏ��Ԃ��g���B
		  u0 = (sec_left / 36) + sec_limit_up + (sec_limit / 72); // �c�莞�ԁ�32 + �b�ǂݎ��� + �������ԁ�64
		  umax = (sec_left / 12) + sec_limit_up + (sec_limit / 48); // �ő�v�l����
		  umin = sec_limit_up + 1;                // �ŏ��v�l����=�b�ǂݎ���+1ms
	  }
	  else if (sec_left > sec_limit && sec_elapsed >= sec_limit) {
		  // �c�莞�Ԃ��������Ԃ�葽���B(�t�B�b�V���[���[�h�p) ����Ԃ��������Ԃ𒴂����ꍇ�B
		  u0 = (sec_left / 33) + sec_limit_up + (sec_limit / 64); // �c�莞�ԁ�32 + �b�ǂݎ��� + �������ԁ�64
		  umax = (sec_left / 11) + sec_limit_up + (sec_limit / 48); // �ő�v�l����
		  umin = sec_limit_up + 1;                // �ŏ��v�l����=�b�ǂݎ���+1ms
	  }
	  else if (sec_left >= sec_limit * 0.90) {
		  // �c�莞�Ԃ��������Ԃ�90���ȏ�B��Ղ𔲂�������ɑ��߂Ɏ��Ԃ��g���B
		  u0 = (sec_left / 30) + (sec_limit_up * 12U) / 10 + (sec_limit / 69); // �c�莞�ԁ�39 + �b�ǂݎ��� + �������ԁ�70
		  umax = (sec_left / 10) + (sec_limit_up * 20U) / 10 + (sec_limit / 32); // �ő�v�l����
		  umin = sec_limit_up + 1;                // �ŏ��v�l����=�b�ǂݎ���+1ms
	  }
	  else if (sec_limit * 0.30 > sec_elapsed /*&& sec_left >= sec_limit*/)
	  {
		  // ����Ԃ��������Ԃ�30%�����B���Ղ͏������Ԃ�ߖ�B
		  u0 = (sec_left / 50) + sec_limit_up + (sec_limit / 192);  // �c�莞�ԁ�80 + �b�ǂݎ��� + �������ԁ�200 
		  umax = (sec_left / 9) + sec_limit_up + (sec_limit / 96); // �ő�v�l����
		  umin = (sec_limit_up * 6U)/10 + 1;           // �ŏ��v�l����=�b�ǂݎ���*0.6
	  }
	  else if (sec_limit * 0.56 > sec_elapsed /*&& sec_left >= sec_limit*/)
	  {
		  // ����Ԃ��������Ԃ�56%�����B���ՂɎ��Ԃ��g���B 
		  u0 = (sec_left / 32) + (sec_limit_up * 2U) + (sec_limit / 64); // �c�莞�ԁ�29 + �b�ǂݎ��� + �������ԁ�80
		  umax = (sec_left /16) + (sec_limit_up * 4U) + (sec_limit / 32); // �ő�v�l����
		  umin = (sec_limit_up * 12U) / 10 + 1;          // �ŏ��v�l����=�b�ǂݎ���*1.2
	  }
	  else if (sec_limit * 0.67 > sec_elapsed /*&& sec_left >= sec_limit*/)
	  {
		  // ����Ԃ��������Ԃ�67%�����B���ՂɎ��Ԃ��g���B 
		  u0 = (sec_left / 26) + (sec_limit_up * 3U) + (sec_limit / 72); //�c�莞�ԁ�24 + �b�ǂݎ��� + �������ԁ�90
		  umax = (sec_left / 13) + (sec_limit_up * 6U) + (sec_limit / 43); // �ő�v�l����
		  umin = (sec_limit_up * 14U) / 10 + 1;         // �ŏ��v�l����=�b�ǂݎ���*1.4
	  }
	  else if (sec_limit * 0.86 > sec_elapsed /*&& sec_left >= sec_limit*/)
	  {
		  // ����Ԃ��������Ԃ�86%�����B���ՂɎ��Ԃ��g���B 
		  u0 = (sec_left / 20) + (sec_limit_up * 4U) + (sec_limit / 78); //�c�莞�ԁ�24 + �b�ǂݎ��� + �������ԁ�90
		  umax = (sec_left / 10) + (sec_limit_up * 8U) + (sec_limit / 54); // �ő�v�l����
		  umin = (sec_limit_up * 16U) / 10 + 1;         // �ŏ��v�l����=�b�ǂݎ���*1.6
	  }
	  else if (sec_limit * 0.95 > sec_elapsed /*&& sec_left >= sec_limit*/)
	  {
		  // ����Ԃ��������Ԃ�95%�����B���I�ՂɎ��Ԃ��g���B 
		  u0 = (sec_left / 14) + (sec_limit_up * 5U) + (sec_limit /86); //�c�莞�ԁ�24 + �b�ǂݎ��� + �������ԁ�90
		  umax = (sec_left / 7) + (sec_limit_up * 10U) + (sec_limit / 70); // �ő�v�l����
		  umin = (sec_limit_up * 18U) / 10 + 1;         // �ŏ��v�l����=�b�ǂݎ���*1.8
	  }
	  else if (sec_limit > sec_elapsed /*&& sec_left >= sec_limit*/) {
		  // ����Ԃ��������Ԃ�100%�����B�I�ՂɎ��Ԃ��g���B 
		 // u0 =  sec_left / 3 + (sec_limit_up * 7) + (sec_limit / 96); //�c�莞�ԁ�3 + �b�ǂݎ���*7 + �������ԁ�90
		  u0 = (sec_left / 8) + (sec_limit_up * 6U) + (sec_limit / 96);
		  umax = (sec_left / 4) + (sec_limit_up * 12U) + (sec_limit / 86); // �ő�v�l����
		  umin = (sec_limit_up * 2U) + 1;         // �ŏ��v�l����=�b�ǂݎ���*2.0
	  }
	  else if (sec_left >= sec_limit * 0.67) {//sec_limit_up * 21) {
		  // (�t�B�b�V���[���[�h�p)�c�莞�Ԃ��������Ԃ�30���ȏ�B���I�ՂɎ��Ԃ��g���B 
		  u0 = (sec_left / 24) + (sec_limit_up * 14U) / 10 + (sec_limit / 102); //�c�莞�ԁ�24 + �b�ǂݎ��� + �������ԁ�90
		  umax = sec_left / 7 + (sec_limit_up * 22U) / 10 + (sec_limit / 96); // �ő�v�l����
		  umin = (sec_limit_up * 5U) / 10 + 1;         // �ŏ��v�l����=�b�ǂݎ���*0.5
	  }
	  else if (sec_left >= sec_limit * 0.33) {//sec_limit_up * 21) {
											  // (�t�B�b�V���[���[�h�p)�c�莞�Ԃ��������Ԃ�30���ȏ�B���I�ՂɎ��Ԃ��g���B 
		  u0 = (sec_left / 18) + (sec_limit_up * 12U) / 10 + (sec_limit / 128); //�c�莞�ԁ�24 + �b�ǂݎ��� + �������ԁ�90
		  umax = sec_left / 6 + (sec_limit_up * 18U) / 10 + (sec_limit / 108); // �ő�v�l����
		  umin = (sec_limit_up * 5U) / 10 + 1;         // �ŏ��v�l����=�b�ǂݎ���*0.5
	  }
	  else if (sec_left >= sec_limit * 0.06) {
		  // (�t�B�b�V���[���[�h�p)�c�莞�Ԃ��������Ԃ�6���ȏ�B�b�ǂݎ��Ԃ��g�����ƑO��̎��Ԕz���ɁB
		  u0 = (sec_left / 12) + sec_limit_up + (sec_limit / 540); // �c�莞�ԁ�12 + �b�ǂݎ��� + �������ԁ�540
		  umax = (sec_left / 5) + (sec_limit_up * 14U) / 10 + (sec_limit / 300);  // �ő�v�l����
		  umin = (sec_limit_up * 4U)/10 + 1;          // �ŏ��v�l����=�b�ǂݎ���*0.4
	  }
	  else if (sec_left > sec_limit * 0.03) {
		  // (�t�B�b�V���[���[�h�p)�c�莞�Ԃ��������Ԃ�3���ȏ�B�b�ǂݎ��Ԃ��g�����ƑO��̎��Ԕz���ɁB
		  u0 = (sec_left / 9) + sec_limit_up; // �c�莞�ԁ�8 + �b�ǂݎ���
		  umax = (sec_left / 6) + sec_limit_up; // �ő�v�l����
		  umin = (sec_limit_up * 3U) / 10 + 1;      // �ŏ��v�l����=�b�ǂݎ���*0.3
	  }
	  // �[���̎c�莞�Ԃ́A�g������
	  else //(sec_left <= sec_limit * 0.03)
	  {
		  u0 = (sec_left * 6)/100 + (sec_limit_up*90U)/100;
		  umax = (sec_left * 12)/100 + (sec_limit_up * 93U) / 100;
		  umin = (sec_left * 3)/100 + (sec_limit_up * 67U) / 100;
	  }

	  u1 = u0 * 5U;

	  umax = sec_left + (sec_limit_up * 90) / 100;
	  umin = 1;

	  if (umax < u0) { u0 = umax; }
	  if (umax < u1) { u1 = umax; }
	  if (umin > u0) { u0 = umin; }
	  if (umin > u1) { u1 = umin; }
  }
  /* no byo-yomi */
  else {
    unsigned int sec_elapsed, sec_left;
    
    sec_elapsed = turn ? sec_w_total : sec_b_total;

    /* We have some seconds left to think. */
    if ( sec_elapsed + SEC_MARGIN < sec_limit )
      {
	sec_left = sec_limit - sec_elapsed;
	u0       = ( sec_left + ( TC_NMOVE / 2U ) ) / TC_NMOVE;
	
	/* t = 2s is not beneficial since 2.8s is almost the same as 1.8s. */
	/* So that, we rather want to save the time.                       */
	if ( u0 == 2U ) { u0 = 1U; }
	u1 = u0 * 5U;
      }
    /* We are running out of time... */
    else { u0 = u1 = 1U; }
  }
  
  time_limit     = u0 * 1000U + 1000U - time_response;
  time_max_limit = u1 * 1000U + 1000U - time_response;
  
  if ( game_status & flag_puzzling )
    {
      time_max_limit = time_max_limit / 5U;
      time_limit     = time_max_limit / 5U;
    }

  Out( "- time ctrl: %u -- %u\n", time_limit, time_max_limit );
}


int CONV
update_time( int turn )
{
  unsigned int te, time_elapsed;
  int iret;

  iret = get_elapsed( &te );
  if ( iret < 0 ) { return iret; }

  time_elapsed = te - time_turn_start;
  sec_elapsed  = time_elapsed / 1000U;
  if ( ! sec_elapsed ) { sec_elapsed = 1U; }

  if ( turn ) { sec_w_total += sec_elapsed; }
  else        { sec_b_total += sec_elapsed; }
  
  time_turn_start = te;

  return 1;
}


void CONV
adjust_time( unsigned int elapsed_new, int turn )
{
  const char *str = "TIME SKEW DETECTED";

  if ( turn )
    {
      if ( sec_w_total + elapsed_new < sec_elapsed )
	{
	  out_warning( str );
	  sec_w_total = 0;
	}
      else { sec_w_total = sec_w_total + elapsed_new - sec_elapsed; };
    }
  else {
    if ( sec_b_total + elapsed_new < sec_elapsed )
      {
	out_warning( str );
	sec_b_total = 0;
      }
    else { sec_b_total = sec_b_total + elapsed_new - sec_elapsed; };
  }
}


int CONV
reset_time( unsigned int b_remain, unsigned int w_remain )
{
  if ( sec_limit_up == UINT_MAX )
    {
      str_error = "no time limit is set";
      return -2;
    }

  if ( b_remain > sec_limit || w_remain > sec_limit )
    {
      snprintf( str_message, SIZE_MESSAGE,
		"time remaining can't be larger than %u", sec_limit );
      str_error = str_message;
      return -2;
    }

  sec_b_total = sec_limit - b_remain;
  sec_w_total = sec_limit - w_remain;
  Out( "  elapsed: b%u, w%u\n", sec_b_total, sec_w_total );

  return 1;
}


const char * CONV
str_time( unsigned int time )
{
  static char str[32];
  unsigned int time_min = time / 60000;
  unsigned int time_sec = time / 1000;
  unsigned int time_mil = time % 1000;

  if ( time_min < 60 )
    {
      snprintf( str, 32, "%02u:%02u.%02u",
		time_min, time_sec%60, time_mil/10 );
    }
  else {
    snprintf( str, 32, "%02u:%02u:%02u.%02u",
	      time_min / 60, time_min % 60, time_sec%60, time_mil/10 );
  }
  return str;
}


const char * CONV
str_time_symple( unsigned int time )
{
  static char str[32];
  unsigned int time_min = time / 60000;
  unsigned int time_sec = time / 1000;
  unsigned int time_mil = time % 1000;

  if ( time_min == 0 )
    {
      snprintf( str, 32, "%5.2f", (float)(time_sec*1000+time_mil)/1000.0 );
    }
  else { snprintf( str, 32, "%2u:%02u", time_min, time_sec % 60 ); }

  return str;
}


int CONV
get_cputime( unsigned int *ptime )
{
#if defined(_WIN32)
  HANDLE         hProcess;
  FILETIME       CreationTime, ExitTime, KernelTime, UserTime;
  ULARGE_INTEGER uli_temp;

  hProcess = GetCurrentProcess();
  if ( GetProcessTimes( hProcess, &CreationTime, &ExitTime,
			&KernelTime, &UserTime ) )
    {
      uli_temp.LowPart  = UserTime.dwLowDateTime;
      uli_temp.HighPart = UserTime.dwHighDateTime;
      *ptime = (unsigned int)( uli_temp.QuadPart / 10000 );
    }
  else {
#  if 0 /* winnt */
    str_error = "GetProcessTimes() failed.";
    return -1;
#  else /* win95 */
    *ptime = 0U;
#  endif
  }
#else
  clock_t clock_temp;
  struct tms t;

  if ( times( &t ) == -1 )
    {
      str_error = "times() faild.";
      return -1;
    }
  clock_temp = t.tms_utime + t.tms_stime + t.tms_cutime + t.tms_cstime;
  *ptime  = (unsigned int)( clock_temp / clk_tck ) * 1000;
  clock_temp %= clk_tck;
  *ptime += (unsigned int)( (clock_temp * 1000) / clk_tck );
#endif /* no _WIN32 */
  
  return 1;
}


int CONV
get_elapsed( unsigned int *ptime )
{
#if defined(_WIN32)
  /*
    try timeBeginPeriod() and timeGetTime(),
    or QueryPerformanceFrequency and QueryPerformanceCounter()
  */
  FILETIME       FileTime;
  ULARGE_INTEGER uli_temp;

  GetSystemTimeAsFileTime( &FileTime );
  uli_temp.LowPart  = FileTime.dwLowDateTime;
  uli_temp.HighPart = FileTime.dwHighDateTime;
  *ptime            = (unsigned int)( uli_temp.QuadPart / 10000U );
#else
  struct timeval timeval;

  if ( gettimeofday( &timeval, NULL ) == -1 )
    {
      str_error = "gettimeofday() faild.";
      return -1;
    }
  *ptime  = (unsigned int)timeval.tv_sec * 1000;
  *ptime += (unsigned int)timeval.tv_usec / 1000;
#endif /* no _WIN32 */

  return 1;
}
