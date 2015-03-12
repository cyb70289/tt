/* Helpers
 *
 * Copyright (C) 2015 Yibo Cai
 */
#pragma once

void _tt_apn_zero(struct tt_apn *apn);

uint _tt_apn_from_d3(const uchar *dec3);
void _tt_apn_to_d3(uint bit10, uchar *dec3);

int _tt_apn_round_adj(struct tt_apn *apn);
