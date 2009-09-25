// This file was generated by qlalr - DO NOT EDIT!
#ifndef EXPRESSIONTABLE_P_H
#define EXPRESSIONTABLE_P_H

class ExpressionTable
{
public:
  enum {
    EOF_SYMBOL = 0,
    otherwise_prec = 19,
    tAdd = 1,
    tAssig = 4,
    tColon = 18,
    tComa = 11,
    tDiv = 6,
    tEq = 17,
    tId = 8,
    tLambda = 9,
    tLcb = 14,
    tLimits = 5,
    tLpr = 12,
    tMul = 3,
    tPow = 7,
    tQm = 10,
    tRcb = 15,
    tRpr = 13,
    tSub = 2,
    tVal = 16,
    uminus_prec = 20,

    ACCEPT_STATE = 66,
    RULE_COUNT = 38,
    STATE_COUNT = 73,
    TERMINAL_COUNT = 21,
    NON_TERMINAL_COUNT = 15,

    GOTO_INDEX_OFFSET = 73,
    GOTO_INFO_OFFSET = 175,
    GOTO_CHECK_OFFSET = 175
  };

  static const char  *const spell [];
  static const int            lhs [];
  static const int            rhs [];

#ifndef QLALR_NO_EXPRESSIONTABLE_DEBUG_INFO
  static const int     rule_index [];
  static const int      rule_info [];
#endif // QLALR_NO_EXPRESSIONTABLE_DEBUG_INFO

  static const int   goto_default [];
  static const int action_default [];
  static const int   action_index [];
  static const int    action_info [];
  static const int   action_check [];

  static inline int nt_action (int state, int nt)
  {
    const int *const goto_index = &action_index [GOTO_INDEX_OFFSET];
    const int *const goto_check = &action_check [GOTO_CHECK_OFFSET];

    const int yyn = goto_index [state] + nt;

    if (yyn < 0 || goto_check [yyn] != nt)
      return goto_default [nt];

    const int *const goto_info = &action_info [GOTO_INFO_OFFSET];
    return goto_info [yyn];
  }

  static inline int t_action (int state, int token)
  {
    const int yyn = action_index [state] + token;

    if (yyn < 0 || action_check [yyn] != token)
      return - action_default [state];

    return action_info [yyn];
  }
};


#endif // EXPRESSIONTABLE_P_H

