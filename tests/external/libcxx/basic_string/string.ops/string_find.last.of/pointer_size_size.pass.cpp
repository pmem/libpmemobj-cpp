//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2019, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "unittest.hpp"

#include <libpmemobj++/container/string.hpp>

namespace nvobj = pmem::obj;

using C = nvobj::string;

struct root {
	nvobj::persistent_ptr<C> s_arr[289];
};

template <class S>
void
test(const S &s, const typename S::value_type *str, typename S::size_type pos,
     typename S::size_type n, typename S::size_type x)
{
	UT_ASSERT(s.find_last_of(str, pos, n) == x);
	if (x != S::npos)
		UT_ASSERT(x <= pos && x < s.size());
}

template <class S>
void
test0(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(*s_arr[0], "", 0, 0, S::npos);
	test(*s_arr[0], "irkhs", 0, 0, S::npos);
	test(*s_arr[0], "kante", 0, 1, S::npos);
	test(*s_arr[0], "oknlr", 0, 2, S::npos);
	test(*s_arr[0], "pcdro", 0, 4, S::npos);
	test(*s_arr[0], "bnrpe", 0, 5, S::npos);
	test(*s_arr[0], "jtdaefblso", 0, 0, S::npos);
	test(*s_arr[0], "oselktgbca", 0, 1, S::npos);
	test(*s_arr[0], "eqgaplhckj", 0, 5, S::npos);
	test(*s_arr[0], "bjahtcmnlp", 0, 9, S::npos);
	test(*s_arr[0], "hjlcmgpket", 0, 10, S::npos);
	test(*s_arr[0], "htaobedqikfplcgjsmrn", 0, 0, S::npos);
	test(*s_arr[0], "hpqiarojkcdlsgnmfetb", 0, 1, S::npos);
	test(*s_arr[0], "dfkaprhjloqetcsimnbg", 0, 10, S::npos);
	test(*s_arr[0], "ihqrfebgadntlpmjksoc", 0, 19, S::npos);
	test(*s_arr[0], "ngtjfcalbseiqrphmkdo", 0, 20, S::npos);
	test(*s_arr[0], "", 1, 0, S::npos);
	test(*s_arr[0], "lbtqd", 1, 0, S::npos);
	test(*s_arr[0], "tboim", 1, 1, S::npos);
	test(*s_arr[0], "slcer", 1, 2, S::npos);
	test(*s_arr[0], "cbjfs", 1, 4, S::npos);
	test(*s_arr[0], "aqibs", 1, 5, S::npos);
	test(*s_arr[0], "gtfblmqinc", 1, 0, S::npos);
	test(*s_arr[0], "mkqpbtdalg", 1, 1, S::npos);
	test(*s_arr[0], "kphatlimcd", 1, 5, S::npos);
	test(*s_arr[0], "pblasqogic", 1, 9, S::npos);
	test(*s_arr[0], "arosdhcfme", 1, 10, S::npos);
	test(*s_arr[0], "blkhjeogicatqfnpdmsr", 1, 0, S::npos);
	test(*s_arr[0], "bmhineprjcoadgstflqk", 1, 1, S::npos);
	test(*s_arr[0], "djkqcmetslnghpbarfoi", 1, 10, S::npos);
	test(*s_arr[0], "lgokshjtpbemarcdqnfi", 1, 19, S::npos);
	test(*s_arr[0], "bqjhtkfepimcnsgrlado", 1, 20, S::npos);
	test(*s_arr[61], "", 0, 0, S::npos);
	test(*s_arr[20], "gfsrt", 0, 0, S::npos);
	test(*s_arr[169], "pfsoc", 0, 1, S::npos);
	test(*s_arr[173], "tpflm", 0, 2, S::npos);
	test(*s_arr[77], "sgkec", 0, 4, 0);
	test(*s_arr[31], "romds", 0, 5, S::npos);
	test(*s_arr[235], "qhjistlgmr", 0, 0, S::npos);
	test(*s_arr[170], "pedfirsglo", 0, 1, S::npos);
	test(*s_arr[126], "aqcoslgrmk", 0, 5, S::npos);
	test(*s_arr[70], "dabckmepqj", 0, 9, 0);
	test(*s_arr[185], "pqscrjthli", 0, 10, S::npos);
	test(*s_arr[279], "kfphdcsjqmobliagtren", 0, 0, S::npos);
	test(*s_arr[25], "rokpefncljibsdhqtagm", 0, 1, S::npos);
	test(*s_arr[108], "afionmkphlebtcjqsgrd", 0, 10, S::npos);
	test(*s_arr[215], "aenmqplidhkofrjbctsg", 0, 19, 0);
	test(*s_arr[29], "osjmbtcadhiklegrpqnf", 0, 20, 0);
	test(*s_arr[43], "", 1, 0, S::npos);
	test(*s_arr[284], "osmia", 1, 0, S::npos);
	test(*s_arr[18], "ckonl", 1, 1, S::npos);
	test(*s_arr[127], "ilcaj", 1, 2, S::npos);
	test(*s_arr[164], "lasiq", 1, 4, S::npos);
	test(*s_arr[281], "kfqmr", 1, 5, S::npos);
	test(*s_arr[136], "klnitfaobg", 1, 0, S::npos);
	test(*s_arr[148], "gjhmdlqikp", 1, 1, S::npos);
	test(*s_arr[100], "skbgtahqej", 1, 5, 0);
	test(*s_arr[58], "bjsdgtlpkf", 1, 9, 0);
	test(*s_arr[28], "bjgfmnlkio", 1, 10, 0);
	test(*s_arr[48], "lbhepotfsjdqigcnamkr", 1, 0, S::npos);
	test(*s_arr[258], "tebangckmpsrqdlfojhi", 1, 1, S::npos);
	test(*s_arr[144], "joflqbdkhtegimscpanr", 1, 10, 1);
	test(*s_arr[68], "adpmcohetfbsrjinlqkg", 1, 19, 1);
	test(*s_arr[27], "iacldqjpfnogbsrhmetk", 1, 20, 1);
	test(*s_arr[203], "", 2, 0, S::npos);
	test(*s_arr[194], "otkgb", 2, 0, S::npos);
	test(*s_arr[243], "cqsjl", 2, 1, S::npos);
	test(*s_arr[274], "dpifl", 2, 2, S::npos);
	test(*s_arr[115], "oapht", 2, 4, 0);
	test(*s_arr[212], "cifts", 2, 5, 1);
	test(*s_arr[119], "nmsckbgalo", 2, 0, S::npos);
	test(*s_arr[90], "tpksqhamle", 2, 1, S::npos);
	test(*s_arr[50], "tpdrchmkji", 2, 5, 2);
	test(*s_arr[125], "ijagfkblst", 2, 9, 2);
	test(*s_arr[130], "kpocsignjb", 2, 10, 0);
	test(*s_arr[64], "pecqtkjsnbdrialgmohf", 2, 0, S::npos);
	test(*s_arr[249], "aiortphfcmkjebgsndql", 2, 1, S::npos);
	test(*s_arr[226], "sdbkeamglhipojqftrcn", 2, 10, 1);
	test(*s_arr[94], "ljqncehgmfktroapidbs", 2, 19, 2);
	test(*s_arr[287], "rtcfodilamkbenjghqps", 2, 20, 2);
	test(*s_arr[40], "", 4, 0, S::npos);
	test(*s_arr[160], "mabns", 4, 0, S::npos);
	test(*s_arr[114], "bdnrp", 4, 1, S::npos);
	test(*s_arr[276], "scidp", 4, 2, S::npos);
	test(*s_arr[229], "agbjl", 4, 4, S::npos);
	test(*s_arr[124], "jfmpr", 4, 5, 4);
	test(*s_arr[158], "rbpefghsmj", 4, 0, S::npos);
	test(*s_arr[227], "apsfntdoqc", 4, 1, S::npos);
	test(*s_arr[280], "ndkjeisgcl", 4, 5, 3);
	test(*s_arr[199], "rnfpqatdeo", 4, 9, 4);
	test(*s_arr[145], "bntjlqrfik", 4, 10, 4);
	test(*s_arr[201], "kcrtmpolnaqejghsfdbi", 4, 0, S::npos);
	test(*s_arr[202], "lobheanpkmqidsrtcfgj", 4, 1, S::npos);
	test(*s_arr[266], "athdkljcnreqbgpmisof", 4, 10, 4);
	test(*s_arr[52], "qkdmjialrscpbhefgont", 4, 19, 4);
	test(*s_arr[95], "dmasojntqleribkgfchp", 4, 20, 4);
	test(*s_arr[163], "", 5, 0, S::npos);
	test(*s_arr[46], "psthd", 5, 0, S::npos);
	test(*s_arr[140], "rpmjd", 5, 1, S::npos);
	test(*s_arr[116], "dfsmk", 5, 2, S::npos);
}

template <class S>
void
test1(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(*s_arr[97], "skqne", 5, 4, 3);
	test(*s_arr[168], "kipnf", 5, 5, 0);
	test(*s_arr[172], "hmrnqdgifl", 5, 0, S::npos);
	test(*s_arr[69], "fsmjcdairn", 5, 1, S::npos);
	test(*s_arr[10], "pcdgltbrfj", 5, 5, 4);
	test(*s_arr[32], "aekfctpirg", 5, 9, 0);
	test(*s_arr[143], "ledihrsgpf", 5, 10, 4);
	test(*s_arr[30], "mqcklahsbtirgopefndj", 5, 0, S::npos);
	test(*s_arr[87], "kmlthaoqgecrnpdbjfis", 5, 1, S::npos);
	test(*s_arr[71], "sfhbamcdptojlkrenqgi", 5, 10, 4);
	test(*s_arr[167], "pbniofmcedrkhlstgaqj", 5, 19, 4);
	test(*s_arr[221], "mongjratcskbhqiepfdl", 5, 20, 4);
	test(*s_arr[96], "", 6, 0, S::npos);
	test(*s_arr[275], "hrnat", 6, 0, S::npos);
	test(*s_arr[283], "gsqdt", 6, 1, S::npos);
	test(*s_arr[60], "bspkd", 6, 2, S::npos);
	test(*s_arr[162], "ohcmb", 6, 4, 2);
	test(*s_arr[19], "heatr", 6, 5, 1);
	test(*s_arr[178], "pmblckedfn", 6, 0, S::npos);
	test(*s_arr[197], "aceqmsrbik", 6, 1, S::npos);
	test(*s_arr[56], "lmbtdehjrn", 6, 5, 3);
	test(*s_arr[155], "teqmcrlgib", 6, 9, 3);
	test(*s_arr[142], "njolbmspac", 6, 10, 4);
	test(*s_arr[244], "pofnhidklamecrbqjgst", 6, 0, S::npos);
	test(*s_arr[261], "jbhckmtgrqnosafedpli", 6, 1, S::npos);
	test(*s_arr[103], "dobntpmqklicsahgjerf", 6, 10, 4);
	test(*s_arr[74], "tpdshainjkbfoemlrgcq", 6, 19, 4);
	test(*s_arr[59], "oldpfgeakrnitscbjmqh", 6, 20, 4);
	test(*s_arr[151], "", 0, 0, S::npos);
	test(*s_arr[55], "rqegt", 0, 0, S::npos);
	test(*s_arr[23], "dashm", 0, 1, S::npos);
	test(*s_arr[273], "jqirk", 0, 2, S::npos);
	test(*s_arr[149], "rckeg", 0, 4, S::npos);
	test(*s_arr[102], "jscie", 0, 5, S::npos);
	test(*s_arr[132], "efsphndliq", 0, 0, S::npos);
	test(*s_arr[135], "gdicosleja", 0, 1, S::npos);
	test(*s_arr[224], "qcpjibosfl", 0, 5, 0);
	test(*s_arr[256], "lrhmefnjcq", 0, 9, 0);
	test(*s_arr[139], "dtablcrseo", 0, 10, S::npos);
	test(*s_arr[272], "apckjsftedbhgomrnilq", 0, 0, S::npos);
	test(*s_arr[278], "pcbrgflehjtiadnsokqm", 0, 1, S::npos);
	test(*s_arr[259], "nsiadegjklhobrmtqcpf", 0, 10, S::npos);
	test(*s_arr[36], "cpmajdqnolikhgsbretf", 0, 19, 0);
	test(*s_arr[92], "jcflkntmgiqrphdosaeb", 0, 20, 0);
	test(*s_arr[268], "", 1, 0, S::npos);
	test(*s_arr[15], "ontrs", 1, 0, S::npos);
	test(*s_arr[159], "pfkna", 1, 1, S::npos);
	test(*s_arr[99], "ekosa", 1, 2, 1);
	test(*s_arr[89], "anqhk", 1, 4, S::npos);
	test(*s_arr[285], "jekca", 1, 5, 1);
	test(*s_arr[150], "ikemsjgacf", 1, 0, S::npos);
	test(*s_arr[171], "arolgsjkhm", 1, 1, S::npos);
	test(*s_arr[138], "oftkbldhre", 1, 5, 1);
	test(*s_arr[218], "gbkqdoeftl", 1, 9, 0);
	test(*s_arr[38], "sqcflrgtim", 1, 10, 1);
	test(*s_arr[288], "fmhbkislrjdpanogqcet", 1, 0, S::npos);
	test(*s_arr[236], "rnioadktqlgpbcjsmhef", 1, 1, S::npos);
	test(*s_arr[105], "oakgtnldpsefihqmjcbr", 1, 10, 1);
	test(*s_arr[241], "gbnaelosidmcjqktfhpr", 1, 19, 1);
	test(*s_arr[14], "akbripjhlosndcmqgfet", 1, 20, 1);
	test(*s_arr[67], "", 5, 0, S::npos);
	test(*s_arr[86], "pijag", 5, 0, S::npos);
	test(*s_arr[183], "jrckd", 5, 1, S::npos);
	test(*s_arr[11], "qcloh", 5, 2, S::npos);
	test(*s_arr[88], "thlmp", 5, 4, 2);
	test(*s_arr[75], "qidmo", 5, 5, 4);
	test(*s_arr[7], "lnegpsjqrd", 5, 0, S::npos);
	test(*s_arr[9], "rjqdablmfs", 5, 1, 5);
	test(*s_arr[133], "enkgpbsjaq", 5, 5, S::npos);
	test(*s_arr[42], "kdsgoaijfh", 5, 9, 5);
	test(*s_arr[141], "trfqgmckbe", 5, 10, 4);
	test(*s_arr[219], "igetsracjfkdnpoblhqm", 5, 0, S::npos);
	test(*s_arr[180], "nqctfaogirshlekbdjpm", 5, 1, S::npos);
	test(*s_arr[26], "csehfgomljdqinbartkp", 5, 10, 5);
	test(*s_arr[220], "qahoegcmplkfsjbdnitr", 5, 19, 5);
	test(*s_arr[79], "dpteiajrqmsognhlfbkc", 5, 20, 5);
	test(*s_arr[269], "", 9, 0, S::npos);
	test(*s_arr[62], "tqbnh", 9, 0, S::npos);
	test(*s_arr[57], "akmle", 9, 1, S::npos);
	test(*s_arr[205], "iqfkm", 9, 2, 6);
	test(*s_arr[246], "tqjsr", 9, 4, 8);
	test(*s_arr[134], "jplqg", 9, 5, 9);
	test(*s_arr[78], "oilnrbcgtj", 9, 0, S::npos);
	test(*s_arr[84], "morkglpesn", 9, 1, 7);
	test(*s_arr[82], "dmicerngat", 9, 5, 9);
	test(*s_arr[182], "radgeskbtc", 9, 9, 6);
	test(*s_arr[193], "ljikprsmqo", 9, 10, 5);
	test(*s_arr[6], "trqihkcgsjamfdbolnpe", 9, 0, S::npos);
	test(*s_arr[17], "lqmthbsrekajgnofcipd", 9, 1, 6);
	test(*s_arr[225], "jtalmedribkgqsopcnfh", 9, 10, 7);
	test(*s_arr[49], "spqfoiclmtagejbndkrh", 9, 19, 9);
	test(*s_arr[54], "nmotklspigjrdhcfaebq", 9, 20, 9);
	test(*s_arr[200], "", 10, 0, S::npos);
	test(*s_arr[117], "hpmsd", 10, 0, S::npos);
	test(*s_arr[233], "qnpor", 10, 1, 1);
	test(*s_arr[107], "otdma", 10, 2, 2);
	test(*s_arr[35], "efhjg", 10, 4, 7);
	test(*s_arr[16], "odpte", 10, 5, 7);
	test(*s_arr[3], "bctdgfmolr", 10, 0, S::npos);
	test(*s_arr[137], "oaklidrbqg", 10, 1, 1);
}

template <class S>
void
test2(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(*s_arr[265], "dnjfsagktr", 10, 5, 9);
	test(*s_arr[111], "nejaktmiqg", 10, 9, 8);
	test(*s_arr[21], "pjqonlebsf", 10, 10, 9);
	test(*s_arr[80], "dshmnbtolcjepgaikfqr", 10, 0, S::npos);
	test(*s_arr[260], "iogfhpabtjkqlrnemcds", 10, 1, 8);
	test(*s_arr[154], "ngridfabjsecpqltkmoh", 10, 10, 9);
	test(*s_arr[267], "athmknplcgofrqejsdib", 10, 19, 9);
	test(*s_arr[240], "ldobhmqcafnjtkeisgrp", 10, 20, 9);
	test(*s_arr[45], "", 11, 0, S::npos);
	test(*s_arr[123], "aocjb", 11, 0, S::npos);
	test(*s_arr[282], "jbrnk", 11, 1, 1);
	test(*s_arr[206], "tqedg", 11, 2, 7);
	test(*s_arr[248], "nqskp", 11, 4, 3);
	test(*s_arr[239], "eaqkl", 11, 5, 9);
	test(*s_arr[122], "reaoicljqm", 11, 0, S::npos);
	test(*s_arr[118], "lsftgajqpm", 11, 1, 1);
	test(*s_arr[214], "rlpfogmits", 11, 5, 7);
	test(*s_arr[152], "shkncmiaqj", 11, 9, 9);
	test(*s_arr[252], "fpnatrhqgs", 11, 10, 9);
	test(*s_arr[277], "sjclemqhnpdbgikarfot", 11, 0, S::npos);
	test(*s_arr[179], "otcmedjikgsfnqbrhpla", 11, 1, S::npos);
	test(*s_arr[39], "bonsaefdqiprkhlgtjcm", 11, 10, 9);
	test(*s_arr[209], "egpscmahijlfnkrodqtb", 11, 19, 9);
	test(*s_arr[113], "kmqbfepjthgilscrndoa", 11, 20, 9);
	test(*s_arr[13], "", 0, 0, S::npos);
	test(*s_arr[66], "prboq", 0, 0, S::npos);
	test(*s_arr[245], "fjcqh", 0, 1, S::npos);
	test(*s_arr[37], "fmosa", 0, 2, S::npos);
	test(*s_arr[207], "qdbok", 0, 4, S::npos);
	test(*s_arr[63], "amslg", 0, 5, S::npos);
	test(*s_arr[174], "smpltjneqb", 0, 0, S::npos);
	test(*s_arr[198], "flitskrnge", 0, 1, S::npos);
	test(*s_arr[34], "pgqihmlbef", 0, 5, S::npos);
	test(*s_arr[187], "cfpdqjtgsb", 0, 9, S::npos);
	test(*s_arr[33], "htpsiaflom", 0, 10, S::npos);
	test(*s_arr[211], "kpjfiaceghsrdtlbnomq", 0, 0, S::npos);
	test(*s_arr[208], "qhtbomidljgafneksprc", 0, 1, S::npos);
	test(*s_arr[51], "nhtjobkcefldimpsaqgr", 0, 10, S::npos);
	test(*s_arr[188], "prabcjfqnoeskilmtgdh", 0, 19, 0);
	test(*s_arr[128], "dtrgmchilkasqoebfpjn", 0, 20, 0);
	test(*s_arr[85], "", 1, 0, S::npos);
	test(*s_arr[176], "sqome", 1, 0, S::npos);
	test(*s_arr[251], "smfte", 1, 1, S::npos);
	test(*s_arr[147], "ciboh", 1, 2, 1);
	test(*s_arr[186], "haois", 1, 4, 1);
	test(*s_arr[286], "abfki", 1, 5, S::npos);
	test(*s_arr[177], "frdkocntmq", 1, 0, S::npos);
	test(*s_arr[181], "oasbpedlnr", 1, 1, S::npos);
	test(*s_arr[76], "kltqmhgand", 1, 5, S::npos);
	test(*s_arr[73], "gdtfjchpmr", 1, 9, 1);
	test(*s_arr[121], "ponmcqblet", 1, 10, 1);
	test(*s_arr[210], "sgphqdnofeiklatbcmjr", 1, 0, S::npos);
	test(*s_arr[12], "ljqprsmigtfoneadckbh", 1, 1, S::npos);
	test(*s_arr[271], "ligeojhafnkmrcsqtbdp", 1, 10, 1);
	test(*s_arr[93], "lsimqfnjarbopedkhcgt", 1, 19, 1);
	test(*s_arr[232], "abedmfjlghniorcqptks", 1, 20, 1);
	test(*s_arr[228], "", 10, 0, S::npos);
	test(*s_arr[104], "hqtoa", 10, 0, S::npos);
	test(*s_arr[192], "cahif", 10, 1, S::npos);
	test(*s_arr[98], "kehis", 10, 2, 7);
	test(*s_arr[106], "kdlmh", 10, 4, 10);
	test(*s_arr[230], "paeql", 10, 5, 6);
	test(*s_arr[4], "aghoqiefnb", 10, 0, S::npos);
	test(*s_arr[196], "jrbqaikpdo", 10, 1, 9);
	test(*s_arr[234], "smjonaeqcl", 10, 5, 5);
	test(*s_arr[165], "eqbdrkcfah", 10, 9, 10);
	test(*s_arr[153], "kapmsienhf", 10, 10, 9);
	test(*s_arr[190], "jpqotrlenfcsbhkaimdg", 10, 0, S::npos);
	test(*s_arr[8], "jlbmhnfgtcqprikeados", 10, 1, S::npos);
	test(*s_arr[131], "stgbhfmdaljnpqoicker", 10, 10, 10);
	test(*s_arr[41], "oihcetflbjagdsrkmqpn", 10, 19, 10);
	test(*s_arr[24], "adtclebmnpjsrqfkigoh", 10, 20, 10);
	test(*s_arr[156], "", 19, 0, S::npos);
	test(*s_arr[47], "beafg", 19, 0, S::npos);
	test(*s_arr[257], "iclat", 19, 1, 16);
	test(*s_arr[184], "rkhnf", 19, 2, 7);
	test(*s_arr[44], "clshq", 19, 4, 16);
	test(*s_arr[157], "dtcoj", 19, 5, 19);
	test(*s_arr[72], "rqosnjmfth", 19, 0, S::npos);
	test(*s_arr[1], "siatdfqglh", 19, 1, 15);
	test(*s_arr[238], "mrlshtpgjq", 19, 5, 17);
	test(*s_arr[2], "adlcskgqjt", 19, 9, 16);
	test(*s_arr[129], "drshcjknaf", 19, 10, 16);
	test(*s_arr[195], "etsaqroinghpkjdlfcbm", 19, 0, S::npos);
	test(*s_arr[189], "sgepdnkqliambtrocfhj", 19, 1, 10);
	test(*s_arr[262], "nlmcjaqgbsortfdihkpe", 19, 10, 19);
	test(*s_arr[231], "racfnpmosldibqkghjet", 19, 19, 19);
	test(*s_arr[213], "fjhdsctkqeiolagrnmbp", 19, 20, 19);
	test(*s_arr[250], "", 20, 0, S::npos);
	test(*s_arr[216], "ejanp", 20, 0, S::npos);
	test(*s_arr[110], "odife", 20, 1, 15);
	test(*s_arr[146], "okaqd", 20, 2, 12);
	test(*s_arr[101], "lcdbi", 20, 4, 19);
	test(*s_arr[22], "fsqbj", 20, 5, 19);
	test(*s_arr[204], "bigdomnplq", 20, 0, S::npos);
	test(*s_arr[91], "apiblotgcd", 20, 1, 3);
	test(*s_arr[253], "acfhdenops", 20, 5, 19);
	test(*s_arr[247], "jopdeamcrk", 20, 9, 19);
	test(*s_arr[81], "trqncbkgmh", 20, 10, 19);
	test(*s_arr[175], "tomglrkencbsfjqpihda", 20, 0, S::npos);
}

template <class S>
void
test3(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(*s_arr[222], "gbkhdnpoietfcmrslajq", 20, 1, 4);
	test(*s_arr[161], "rtfnmbsglkjaichoqedp", 20, 10, 17);
	test(*s_arr[270], "ohkmdpfqbsacrtjnlgei", 20, 19, 19);
	test(*s_arr[254], "dlbrteoisgphmkncajfq", 20, 20, 19);
	test(*s_arr[65], "", 21, 0, S::npos);
	test(*s_arr[83], "sjrlo", 21, 0, S::npos);
	test(*s_arr[255], "qjpor", 21, 1, 6);
	test(*s_arr[109], "odhfn", 21, 2, 13);
	test(*s_arr[264], "qtfin", 21, 4, 10);
	test(*s_arr[191], "hpqfo", 21, 5, 17);
	test(*s_arr[217], "fabmertkos", 21, 0, S::npos);
	test(*s_arr[263], "brqtgkmaej", 21, 1, 14);
	test(*s_arr[53], "nfrdeihsgl", 21, 5, 19);
	test(*s_arr[5], "hlfrosekpi", 21, 9, 14);
	test(*s_arr[112], "atgbkrjdsm", 21, 10, 16);
	test(*s_arr[223], "blnrptjgqmaifsdkhoec", 21, 0, S::npos);
	test(*s_arr[242], "ctpmdahebfqjgknloris", 21, 1, 17);
	test(*s_arr[237], "apnkeqthrmlbfodiscgj", 21, 10, 17);
	test(*s_arr[166], "jdgictpframeoqlsbknh", 21, 19, 19);
	test(*s_arr[120], "qprlsfojamgndekthibc", 21, 20, 19);
}

int
main(int argc, char *argv[])
{
	START();

	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " file-name" << std::endl;
		return 1;
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "string_test", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto &s_arr = pop.root()->s_arr;

	try {
		nvobj::transaction::run(pop, [&] {
			s_arr[0] = nvobj::make_persistent<C>("");
			s_arr[1] = nvobj::make_persistent<C>(
				"abqjcfedgotihlnspkrm");
			s_arr[2] = nvobj::make_persistent<C>(
				"abseghclkjqifmtodrnp");
			s_arr[3] = nvobj::make_persistent<C>("adtkqpbjfi");
			s_arr[4] = nvobj::make_persistent<C>(
				"aftsijrbeklnmcdqhgop");
			s_arr[5] = nvobj::make_persistent<C>(
				"ahegrmqnoiklpfsdbcjt");
			s_arr[6] = nvobj::make_persistent<C>("ahlcifdqgs");
			s_arr[7] = nvobj::make_persistent<C>("apcnsibger");
			s_arr[8] = nvobj::make_persistent<C>(
				"apoklnefbhmgqcdrisjt");
			s_arr[9] = nvobj::make_persistent<C>("aqkocrbign");
			s_arr[10] = nvobj::make_persistent<C>("armql");
			s_arr[11] = nvobj::make_persistent<C>("astedncjhk");
			s_arr[12] = nvobj::make_persistent<C>(
				"atjgfsdlpobmeiqhncrk");
			s_arr[13] = nvobj::make_persistent<C>(
				"atqirnmekfjolhpdsgcb");
			s_arr[14] = nvobj::make_persistent<C>("bdnpfcqaem");
			s_arr[15] = nvobj::make_persistent<C>("bdoshlmfin");
			s_arr[16] = nvobj::make_persistent<C>("beanrfodgj");
			s_arr[17] = nvobj::make_persistent<C>("bgjemaltks");
			s_arr[18] = nvobj::make_persistent<C>("bgstp");
			s_arr[19] = nvobj::make_persistent<C>("bhlki");
			s_arr[20] = nvobj::make_persistent<C>("binja");
			s_arr[21] = nvobj::make_persistent<C>("bmeqgcdorj");
			s_arr[22] = nvobj::make_persistent<C>(
				"bmhldogtckrfsanijepq");
			s_arr[23] = nvobj::make_persistent<C>("bmjlpkiqde");
			s_arr[24] = nvobj::make_persistent<C>(
				"bnlgapfimcoterskqdjh");
			s_arr[25] = nvobj::make_persistent<C>("bocjs");
			s_arr[26] = nvobj::make_persistent<C>("bpjlgmiedh");
			s_arr[27] = nvobj::make_persistent<C>("brfsm");
			s_arr[28] = nvobj::make_persistent<C>("bthpg");
			s_arr[29] = nvobj::make_persistent<C>("btlfi");
			s_arr[30] = nvobj::make_persistent<C>("cbrkp");
			s_arr[31] = nvobj::make_persistent<C>("cdafr");
			s_arr[32] = nvobj::make_persistent<C>("cdhjo");
			s_arr[33] = nvobj::make_persistent<C>(
				"ceatbhlsqjgpnokfrmdi");
			s_arr[34] = nvobj::make_persistent<C>(
				"cehkbngtjoiflqapsmrd");
			s_arr[35] = nvobj::make_persistent<C>("cfkqpjlegi");
			s_arr[36] = nvobj::make_persistent<C>("cfpegndlkt");
			s_arr[37] = nvobj::make_persistent<C>(
				"chamfknorbedjitgslpq");
			s_arr[38] = nvobj::make_persistent<C>("cigfqkated");
			s_arr[39] = nvobj::make_persistent<C>("cipogdskjf");
			s_arr[40] = nvobj::make_persistent<C>("cjgao");
			s_arr[41] = nvobj::make_persistent<C>(
				"ckqhaiesmjdnrgolbtpf");
			s_arr[42] = nvobj::make_persistent<C>("clobgsrken");
			s_arr[43] = nvobj::make_persistent<C>("clrgb");
			s_arr[44] = nvobj::make_persistent<C>(
				"cmlfakiojdrgtbsphqen");
			s_arr[45] = nvobj::make_persistent<C>("cqjohampgd");
			s_arr[46] = nvobj::make_persistent<C>("dajhn");
			s_arr[47] = nvobj::make_persistent<C>(
				"dfkechomjapgnslbtqir");
			s_arr[48] = nvobj::make_persistent<C>("dgsnq");
			s_arr[49] = nvobj::make_persistent<C>("dirhtsnjkc");
			s_arr[50] = nvobj::make_persistent<C>("dirnm");
			s_arr[51] = nvobj::make_persistent<C>(
				"dkclqfombepritjnghas");
			s_arr[52] = nvobj::make_persistent<C>("dktbn");
			s_arr[53] = nvobj::make_persistent<C>(
				"dlmsipcnekhbgoaftqjr");
			s_arr[54] = nvobj::make_persistent<C>("dlroktbcja");
			s_arr[55] = nvobj::make_persistent<C>("dltjfngbko");
			s_arr[56] = nvobj::make_persistent<C>("dpqbr");
			s_arr[57] = nvobj::make_persistent<C>("dqmregkcfl");
			s_arr[58] = nvobj::make_persistent<C>("dqtlg");
			s_arr[59] = nvobj::make_persistent<C>("dsnmg");
			s_arr[60] = nvobj::make_persistent<C>("dthpe");
			s_arr[61] = nvobj::make_persistent<C>("eaint");
			s_arr[62] = nvobj::make_persistent<C>("ebcinjgads");
			s_arr[63] = nvobj::make_persistent<C>(
				"ebnghfsqkprmdcljoiat");
			s_arr[64] = nvobj::make_persistent<C>("ebrgd");
			s_arr[65] = nvobj::make_persistent<C>(
				"ecgdanriptblhjfqskom");
			s_arr[66] = nvobj::make_persistent<C>(
				"echfkmlpribjnqsaogtd");
			s_arr[67] = nvobj::make_persistent<C>("ectnhskflp");
			s_arr[68] = nvobj::make_persistent<C>("edapb");
			s_arr[69] = nvobj::make_persistent<C>("egmjk");
			s_arr[70] = nvobj::make_persistent<C>("ehmja");
			s_arr[71] = nvobj::make_persistent<C>("ejfcd");
			s_arr[72] = nvobj::make_persistent<C>(
				"eldiqckrnmtasbghjfpo");
			s_arr[73] = nvobj::make_persistent<C>(
				"emgasrilpknqojhtbdcf");
			s_arr[74] = nvobj::make_persistent<C>("eopfi");
			s_arr[75] = nvobj::make_persistent<C>("epfhocmdng");
			s_arr[76] = nvobj::make_persistent<C>(
				"epoiqmtldrabnkjhcfsg");
			s_arr[77] = nvobj::make_persistent<C>("eqkst");
			s_arr[78] = nvobj::make_persistent<C>("ersmicafdh");
			s_arr[79] = nvobj::make_persistent<C>("espogqbthk");
			s_arr[80] = nvobj::make_persistent<C>("etqlcanmob");
			s_arr[81] = nvobj::make_persistent<C>(
				"fbkeiopclstmdqranjhg");
			s_arr[82] = nvobj::make_persistent<C>("fdbicojerm");
			s_arr[83] = nvobj::make_persistent<C>(
				"fdmiarlpgcskbhoteqjn");
			s_arr[84] = nvobj::make_persistent<C>("fdnplotmgh");
			s_arr[85] = nvobj::make_persistent<C>(
				"febhmqtjanokscdirpgl");
			s_arr[86] = nvobj::make_persistent<C>("fgtianblpq");
			s_arr[87] = nvobj::make_persistent<C>("fhgna");
			s_arr[88] = nvobj::make_persistent<C>("fhlqgcajbr");
			s_arr[89] = nvobj::make_persistent<C>("fjiknedcpq");
			s_arr[90] = nvobj::make_persistent<C>("fklad");
			s_arr[91] = nvobj::make_persistent<C>(
				"focalnrpiqmdkstehbjg");
			s_arr[92] = nvobj::make_persistent<C>("fqbtnkeasj");
			s_arr[93] = nvobj::make_persistent<C>(
				"fraghmbiceknltjpqosd");
			s_arr[94] = nvobj::make_persistent<C>("frehn");
			s_arr[95] = nvobj::make_persistent<C>("fthqm");
			s_arr[96] = nvobj::make_persistent<C>("gajqn");
			s_arr[97] = nvobj::make_persistent<C>("gbhqo");
			s_arr[98] = nvobj::make_persistent<C>(
				"gckarqnelodfjhmbptis");
			s_arr[99] = nvobj::make_persistent<C>("getcrsaoji");
			s_arr[100] = nvobj::make_persistent<C>("gfcql");
			s_arr[101] = nvobj::make_persistent<C>(
				"gftenihpmslrjkqadcob");
			s_arr[102] = nvobj::make_persistent<C>("ghasdbnjqo");
			s_arr[103] = nvobj::make_persistent<C>("ghknq");
			s_arr[104] = nvobj::make_persistent<C>(
				"gjdkeprctqblnhiafsom");
			s_arr[105] = nvobj::make_persistent<C>("gltkojeipd");
			s_arr[106] = nvobj::make_persistent<C>(
				"gqpskidtbclomahnrjfe");
			s_arr[107] = nvobj::make_persistent<C>("gqtjsbdckh");
			s_arr[108] = nvobj::make_persistent<C>("grbsd");
			s_arr[109] = nvobj::make_persistent<C>(
				"grjpqmbshektdolcafni");
			s_arr[110] = nvobj::make_persistent<C>(
				"grkpahljcftesdmonqib");
			s_arr[111] = nvobj::make_persistent<C>("gtfbdkqeml");
			s_arr[112] = nvobj::make_persistent<C>(
				"hdsjbnmlegtkqripacof");
			s_arr[113] = nvobj::make_persistent<C>("hefnrkmctj");
			s_arr[114] = nvobj::make_persistent<C>("herni");
			s_arr[115] = nvobj::make_persistent<C>("hjeni");
			s_arr[116] = nvobj::make_persistent<C>("hkjae");
			s_arr[117] = nvobj::make_persistent<C>("hlbosgmrak");
			s_arr[118] = nvobj::make_persistent<C>("hlmgabenti");
			s_arr[119] = nvobj::make_persistent<C>("hmftq");
			s_arr[120] = nvobj::make_persistent<C>(
				"hnbrcplsjfgiktoedmaq");
			s_arr[121] = nvobj::make_persistent<C>(
				"hnfiagdpcklrjetqbsom");
			s_arr[122] = nvobj::make_persistent<C>("hnprfgqjdl");
			s_arr[123] = nvobj::make_persistent<C>("hobitmpsan");
			s_arr[124] = nvobj::make_persistent<C>("hoser");
			s_arr[125] = nvobj::make_persistent<C>("hrgdc");
			s_arr[126] = nvobj::make_persistent<C>("hrlpd");
			s_arr[127] = nvobj::make_persistent<C>("hstrk");
			s_arr[128] = nvobj::make_persistent<C>(
				"htbcigojaqmdkfrnlsep");
			s_arr[129] = nvobj::make_persistent<C>(
				"ibmsnlrjefhtdokacqpg");
			s_arr[130] = nvobj::make_persistent<C>("ifakg");
			s_arr[131] = nvobj::make_persistent<C>(
				"ifeopcnrjbhkdgatmqls");
			s_arr[132] = nvobj::make_persistent<C>("igrkhpbqjt");
			s_arr[133] = nvobj::make_persistent<C>("ijsmdtqgce");
			s_arr[134] = nvobj::make_persistent<C>("ikabsjtdfl");
			s_arr[135] = nvobj::make_persistent<C>("ikthdgcamf");
			s_arr[136] = nvobj::make_persistent<C>("ilbcj");
			s_arr[137] = nvobj::make_persistent<C>("iomkfthagj");
			s_arr[138] = nvobj::make_persistent<C>("itfsmcjorl");
			s_arr[139] = nvobj::make_persistent<C>("itphbqsker");
			s_arr[140] = nvobj::make_persistent<C>("jbgno");
			s_arr[141] = nvobj::make_persistent<C>("jbhcfposld");
			s_arr[142] = nvobj::make_persistent<C>("jblqp");
			s_arr[143] = nvobj::make_persistent<C>("jcons");
			s_arr[144] = nvobj::make_persistent<C>("jfdam");
			s_arr[145] = nvobj::make_persistent<C>("jgmib");
			s_arr[146] = nvobj::make_persistent<C>(
				"jimlgbhfqkteospardcn");
			s_arr[147] = nvobj::make_persistent<C>(
				"jitlfrqemsdhkopncabg");
			s_arr[148] = nvobj::make_persistent<C>("jkngf");
			s_arr[149] = nvobj::make_persistent<C>("jkpldtshrm");
			s_arr[150] = nvobj::make_persistent<C>("jnakolqrde");
			s_arr[151] = nvobj::make_persistent<C>("jnkrfhotgl");
			s_arr[152] = nvobj::make_persistent<C>("jqedtkornm");
			s_arr[153] = nvobj::make_persistent<C>(
				"jrlbothiknqmdgcfasep");
			s_arr[154] = nvobj::make_persistent<C>("kadsithljf");
			s_arr[155] = nvobj::make_persistent<C>("kdhmo");
			s_arr[156] = nvobj::make_persistent<C>(
				"kgdlrobpmjcthqsafeni");
			s_arr[157] = nvobj::make_persistent<C>(
				"kghbfipeomsntdalrqjc");
			s_arr[158] = nvobj::make_persistent<C>("kgrsp");
			s_arr[159] = nvobj::make_persistent<C>("khfrebnsgq");
			s_arr[160] = nvobj::make_persistent<C>("kjplq");
			s_arr[161] = nvobj::make_persistent<C>(
				"klchabsimetjnqgorfpd");
			s_arr[162] = nvobj::make_persistent<C>("klhde");
			s_arr[163] = nvobj::make_persistent<C>("klopi");
			s_arr[164] = nvobj::make_persistent<C>("kmspj");
			s_arr[165] = nvobj::make_persistent<C>(
				"kpdbgjmtherlsfcqoina");
			s_arr[166] = nvobj::make_persistent<C>(
				"kpfegbjhsrnodltqciam");
			s_arr[167] = nvobj::make_persistent<C>("kqjhe");
			s_arr[168] = nvobj::make_persistent<C>("ktdor");
			s_arr[169] = nvobj::make_persistent<C>("latkm");
			s_arr[170] = nvobj::make_persistent<C>("lbisk");
			s_arr[171] = nvobj::make_persistent<C>("lcjptsmgbe");
			s_arr[172] = nvobj::make_persistent<C>("ldprn");
			s_arr[173] = nvobj::make_persistent<C>("lecfr");
			s_arr[174] = nvobj::make_persistent<C>(
				"letjomsgihfrpqbkancd");
			s_arr[175] = nvobj::make_persistent<C>(
				"lifhpdgmbconstjeqark");
			s_arr[176] = nvobj::make_persistent<C>(
				"loakbsqjpcrdhftniegm");
			s_arr[177] = nvobj::make_persistent<C>(
				"lpfmctjrhdagneskbqoi");
			s_arr[178] = nvobj::make_persistent<C>("lqmoh");
			s_arr[179] = nvobj::make_persistent<C>("lrkcbtqpie");
			s_arr[180] = nvobj::make_persistent<C>("lroeasctif");
			s_arr[181] = nvobj::make_persistent<C>(
				"lsmqaepkdhncirbtjfgo");
			s_arr[182] = nvobj::make_persistent<C>("mbtafndjcq");
			s_arr[183] = nvobj::make_persistent<C>("mfeqklirnh");
			s_arr[184] = nvobj::make_persistent<C>(
				"mgjhkolrnadqbpetcifs");
			s_arr[185] = nvobj::make_persistent<C>("mhqgd");
			s_arr[186] = nvobj::make_persistent<C>(
				"mhtaepscdnrjqgbkifol");
			s_arr[187] = nvobj::make_persistent<C>(
				"mignapfoklbhcqjetdrs");
			s_arr[188] = nvobj::make_persistent<C>(
				"miklnresdgbhqcojftap");
			s_arr[189] = nvobj::make_persistent<C>(
				"mjkticdeoqshpalrfbgn");
			s_arr[190] = nvobj::make_persistent<C>(
				"mjogldqferckabinptsh");
			s_arr[191] = nvobj::make_persistent<C>(
				"mjtdglasihqpocebrfkn");
			s_arr[192] = nvobj::make_persistent<C>(
				"mkpnblfdsahrcqijteog");
			s_arr[193] = nvobj::make_persistent<C>("mlenkpfdtc");
			s_arr[194] = nvobj::make_persistent<C>("mrecp");
			s_arr[195] = nvobj::make_persistent<C>(
				"mrkfciqjebaponsthldg");
			s_arr[196] = nvobj::make_persistent<C>(
				"mtlgdrhafjkbiepqnsoc");
			s_arr[197] = nvobj::make_persistent<C>("mtqin");
			s_arr[198] = nvobj::make_persistent<C>(
				"nblgoipcrqeaktshjdmf");
			s_arr[199] = nvobj::make_persistent<C>("nbmit");
			s_arr[200] = nvobj::make_persistent<C>("ncjpmaekbs");
			s_arr[201] = nvobj::make_persistent<C>("ncrfj");
			s_arr[202] = nvobj::make_persistent<C>("ncsik");
			s_arr[203] = nvobj::make_persistent<C>("ndrhl");
			s_arr[204] = nvobj::make_persistent<C>(
				"nfqkrpjdesabgtlcmoih");
			s_arr[205] = nvobj::make_persistent<C>("ngcrieqajf");
			s_arr[206] = nvobj::make_persistent<C>("ngfbojitcl");
			s_arr[207] = nvobj::make_persistent<C>(
				"njhqpibfmtlkaecdrgso");
			s_arr[208] = nvobj::make_persistent<C>(
				"noelgschdtbrjfmiqkap");
			s_arr[209] = nvobj::make_persistent<C>("nqedcojahi");
			s_arr[210] = nvobj::make_persistent<C>(
				"nsdfebgajhmtricpoklq");
			s_arr[211] = nvobj::make_persistent<C>(
				"ocihkjgrdelpfnmastqb");
			s_arr[212] = nvobj::make_persistent<C>("ocmfj");
			s_arr[213] = nvobj::make_persistent<C>(
				"oegalhmstjrfickpbndq");
			s_arr[214] = nvobj::make_persistent<C>("ofcjanmrbs");
			s_arr[215] = nvobj::make_persistent<C>("ofjqr");
			s_arr[216] = nvobj::make_persistent<C>(
				"ofkqbnjetrmsaidphglc");
			s_arr[217] = nvobj::make_persistent<C>(
				"okaplfrntghqbmeicsdj");
			s_arr[218] = nvobj::make_persistent<C>("omchkfrjea");
			s_arr[219] = nvobj::make_persistent<C>("oqnpblhide");
			s_arr[220] = nvobj::make_persistent<C>("pamkeoidrj");
			s_arr[221] = nvobj::make_persistent<C>("pbdjl");
			s_arr[222] = nvobj::make_persistent<C>(
				"pboqganrhedjmltsicfk");
			s_arr[223] = nvobj::make_persistent<C>(
				"pcnedrfjihqbalkgtoms");
			s_arr[224] = nvobj::make_persistent<C>("pcofgeniam");
			s_arr[225] = nvobj::make_persistent<C>("pdhslbqrfc");
			s_arr[226] = nvobj::make_persistent<C>("peqmt");
			s_arr[227] = nvobj::make_persistent<C>("pgejb");
			s_arr[228] = nvobj::make_persistent<C>(
				"pihgmoeqtnakrjslcbfd");
			s_arr[229] = nvobj::make_persistent<C>("pkfeo");
			s_arr[230] = nvobj::make_persistent<C>(
				"pkldjsqrfgitbhmaecno");
			s_arr[231] = nvobj::make_persistent<C>(
				"plkqbhmtfaeodjcrsing");
			s_arr[232] = nvobj::make_persistent<C>(
				"pmafenlhqtdbkirjsogc");
			s_arr[233] = nvobj::make_persistent<C>("pqfhsgilen");
			s_arr[234] = nvobj::make_persistent<C>(
				"pqgirnaefthokdmbsclj");
			s_arr[235] = nvobj::make_persistent<C>("prbhe");
			s_arr[236] = nvobj::make_persistent<C>("qcpaemsinf");
			s_arr[237] = nvobj::make_persistent<C>(
				"qeindtagmokpfhsclrbj");
			s_arr[238] = nvobj::make_persistent<C>(
				"qfbadrtjsimkolcenhpg");
			s_arr[239] = nvobj::make_persistent<C>("qghptonrea");
			s_arr[240] = nvobj::make_persistent<C>("qgmetnabkl");
			s_arr[241] = nvobj::make_persistent<C>("qistfrgnmp");
			s_arr[242] = nvobj::make_persistent<C>(
				"qjidealmtpskrbfhocng");
			s_arr[243] = nvobj::make_persistent<C>("qlasf");
			s_arr[244] = nvobj::make_persistent<C>("qmjgl");
			s_arr[245] = nvobj::make_persistent<C>(
				"qnhiftdgcleajbpkrosm");
			s_arr[246] = nvobj::make_persistent<C>("qosmilgnjb");
			s_arr[247] = nvobj::make_persistent<C>(
				"rahdtmsckfboqlpniegj");
			s_arr[248] = nvobj::make_persistent<C>("rcfkdbhgjo");
			s_arr[249] = nvobj::make_persistent<C>("rcjml");
			s_arr[250] = nvobj::make_persistent<C>(
				"rdtgjcaohpblniekmsfq");
			s_arr[251] = nvobj::make_persistent<C>(
				"reagphsqflbitdcjmkno");
			s_arr[252] = nvobj::make_persistent<C>("rfedlasjmg");
			s_arr[253] = nvobj::make_persistent<C>(
				"rhqdspkmebiflcotnjga");
			s_arr[254] = nvobj::make_persistent<C>(
				"rlbdsiceaonqjtfpghkm");
			s_arr[255] = nvobj::make_persistent<C>(
				"rlbstjqopignecmfadkh");
			s_arr[256] = nvobj::make_persistent<C>("rlfjgesqhc");
			s_arr[257] = nvobj::make_persistent<C>(
				"rloadknfbqtgmhcsipje");
			s_arr[258] = nvobj::make_persistent<C>("rmfhp");
			s_arr[259] = nvobj::make_persistent<C>("rocfeldqpk");
			s_arr[260] = nvobj::make_persistent<C>("roqmkbdtia");
			s_arr[261] = nvobj::make_persistent<C>("rothp");
			s_arr[262] = nvobj::make_persistent<C>(
				"rqnoclbdejgiphtfsakm");
			s_arr[263] = nvobj::make_persistent<C>(
				"sahngemrtcjidqbklfpo");
			s_arr[264] = nvobj::make_persistent<C>(
				"sakfcohtqnibprjmlged");
			s_arr[265] = nvobj::make_persistent<C>("sdpcilonqj");
			s_arr[266] = nvobj::make_persistent<C>("sgbfh");
			s_arr[267] = nvobj::make_persistent<C>("sgtkpbfdmh");
			s_arr[268] = nvobj::make_persistent<C>("shbcqnmoar");
			s_arr[269] = nvobj::make_persistent<C>("shoiedtcjb");
			s_arr[270] = nvobj::make_persistent<C>(
				"sirfgmjqhctndbklaepo");
			s_arr[271] = nvobj::make_persistent<C>(
				"sitodfgnrejlahcbmqkp");
			s_arr[272] = nvobj::make_persistent<C>("skjafcirqm");
			s_arr[273] = nvobj::make_persistent<C>("skrflobnqm");
			s_arr[274] = nvobj::make_persistent<C>("smaqd");
			s_arr[275] = nvobj::make_persistent<C>("stedk");
			s_arr[276] = nvobj::make_persistent<C>("tadrb");
			s_arr[277] = nvobj::make_persistent<C>("talpqjsgkm");
			s_arr[278] = nvobj::make_persistent<C>("tcqomarsfd");
			s_arr[279] = nvobj::make_persistent<C>("tgklq");
			s_arr[280] = nvobj::make_persistent<C>("thlnq");
			s_arr[281] = nvobj::make_persistent<C>("tjboh");
			s_arr[282] = nvobj::make_persistent<C>("tjehkpsalm");
			s_arr[283] = nvobj::make_persistent<C>("tjkaf");
			s_arr[284] = nvobj::make_persistent<C>("tjmek");
			s_arr[285] = nvobj::make_persistent<C>("tkejgnafrm");
			s_arr[286] = nvobj::make_persistent<C>(
				"tocesrfmnglpbjihqadk");
			s_arr[287] = nvobj::make_persistent<C>("tqolf");
			s_arr[288] = nvobj::make_persistent<C>("tscenjikml");
		});

		test0<C>(pop);
		test1<C>(pop);
		test2<C>(pop);
		test3<C>(pop);

		nvobj::transaction::run(pop, [&] {
			for (unsigned i = 0; i < 289; ++i) {
				nvobj::delete_persistent<C>(s_arr[i]);
			}
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
