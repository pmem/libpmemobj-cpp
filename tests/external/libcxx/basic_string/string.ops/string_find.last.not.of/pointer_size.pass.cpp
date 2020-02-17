//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2019-2020, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "unittest.hpp"

#include <libpmemobj++/container/string.hpp>

namespace nvobj = pmem::obj;

using C = nvobj::string;

struct root {
	nvobj::persistent_ptr<C> s_arr[85];
};

template <class S>
void
test(const S &s, const typename S::value_type *str, typename S::size_type pos,
     typename S::size_type x)
{
	UT_ASSERT(s.find_last_not_of(str, pos) == x);
	if (x != S::npos)
		UT_ASSERT(x <= pos && x < s.size());
}

template <class S>
void
test(const S &s, const typename S::value_type *str, typename S::size_type x)
{
	UT_ASSERT(s.find_last_not_of(str) == x);
	if (x != S::npos)
		UT_ASSERT(x < s.size());
}

template <class S>
void
test0(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(*s_arr[0], "", 0, S::npos);
	test(*s_arr[0], "laenf", 0, S::npos);
	test(*s_arr[0], "pqlnkmbdjo", 0, S::npos);
	test(*s_arr[0], "qkamfogpnljdcshbreti", 0, S::npos);
	test(*s_arr[0], "", 1, S::npos);
	test(*s_arr[0], "bjaht", 1, S::npos);
	test(*s_arr[0], "hjlcmgpket", 1, S::npos);
	test(*s_arr[0], "htaobedqikfplcgjsmrn", 1, S::npos);
	test(*s_arr[24], "", 0, 0);
	test(*s_arr[71], "dfkap", 0, 0);
	test(*s_arr[7], "ihqrfebgad", 0, 0);
	test(*s_arr[53], "ngtjfcalbseiqrphmkdo", 0, S::npos);
	test(*s_arr[82], "", 1, 1);
	test(*s_arr[62], "ikcrq", 1, 1);
	test(*s_arr[6], "dmajblfhsg", 1, 0);
	test(*s_arr[75], "oqftjhdmkgsblacenirp", 1, S::npos);
	test(*s_arr[12], "", 2, 2);
	test(*s_arr[47], "oebqi", 2, 2);
	test(*s_arr[74], "kojhpmbsfe", 2, 1);
	test(*s_arr[67], "acbsjqogpltdkhinfrem", 2, S::npos);
	test(*s_arr[23], "", 4, 4);
	test(*s_arr[41], "aobjd", 4, 4);
	test(*s_arr[66], "pcbahntsje", 4, 4);
	test(*s_arr[55], "fhepcrntkoagbmldqijs", 4, S::npos);
	test(*s_arr[20], "", 5, 4);
	test(*s_arr[63], "kocgb", 5, 4);
	test(*s_arr[64], "fbslrjiqkm", 5, 4);
	test(*s_arr[65], "jeidpcmalhfnqbgtrsko", 5, S::npos);
	test(*s_arr[79], "", 6, 4);
	test(*s_arr[36], "qngpd", 6, 4);
	test(*s_arr[5], "rodhqklgmb", 6, S::npos);
	test(*s_arr[84], "thdjgafrlbkoiqcspmne", 6, S::npos);
	test(*s_arr[31], "", 0, 0);
	test(*s_arr[13], "ashjd", 0, S::npos);
	test(*s_arr[50], "mgojkldsqh", 0, S::npos);
	test(*s_arr[2], "imqnaghkfrdtlopbjesc", 0, S::npos);
	test(*s_arr[76], "", 1, 1);
	test(*s_arr[26], "nadkh", 1, 1);
	test(*s_arr[60], "ofdrqmkebl", 1, 0);
	test(*s_arr[25], "bdfjqgatlksriohemnpc", 1, S::npos);
	test(*s_arr[10], "", 5, 5);
	test(*s_arr[39], "prqgn", 5, 5);
	test(*s_arr[73], "pejafmnokr", 5, 4);
	test(*s_arr[9], "odnqkgijrhabfmcestlp", 5, S::npos);
	test(*s_arr[51], "", 9, 9);
	test(*s_arr[35], "rtjpa", 9, 8);
	test(*s_arr[16], "ktsrmnqagd", 9, 9);
	test(*s_arr[52], "rtdhgcisbnmoaqkfpjle", 9, S::npos);
	test(*s_arr[18], "", 10, 9);
	test(*s_arr[54], "dplqa", 10, 9);
	test(*s_arr[43], "dkacjoptns", 10, 9);
	test(*s_arr[14], "hqfimtrgnbekpdcsjalo", 10, S::npos);
	test(*s_arr[21], "", 11, 9);
	test(*s_arr[3], "lofbc", 11, 9);
	test(*s_arr[34], "astoegbfpn", 11, 8);
	test(*s_arr[83], "pdgreqomsncafklhtibj", 11, S::npos);
	test(*s_arr[80], "", 0, 0);
	test(*s_arr[1], "lbtqd", 0, 0);
	test(*s_arr[70], "tboimldpjh", 0, S::npos);
	test(*s_arr[15], "slcerthdaiqjfnobgkpm", 0, S::npos);
	test(*s_arr[38], "", 1, 1);
	test(*s_arr[48], "aqibs", 1, 1);
	test(*s_arr[77], "gtfblmqinc", 1, 0);
	test(*s_arr[29], "mkqpbtdalgniorhfescj", 1, S::npos);
	test(*s_arr[32], "", 10, 10);
	test(*s_arr[40], "pblas", 10, 9);
	test(*s_arr[22], "arosdhcfme", 10, 9);
	test(*s_arr[11], "blkhjeogicatqfnpdmsr", 10, S::npos);
	test(*s_arr[59], "", 19, 19);
	test(*s_arr[8], "djkqc", 19, 19);
	test(*s_arr[56], "lgokshjtpb", 19, 16);
	test(*s_arr[42], "bqjhtkfepimcnsgrlado", 19, S::npos);
	test(*s_arr[17], "", 20, 19);
	test(*s_arr[27], "nocfa", 20, 18);
	test(*s_arr[81], "bgtajmiedc", 20, 19);
	test(*s_arr[78], "lsckfnqgdahejiopbtmr", 20, S::npos);
	test(*s_arr[49], "", 21, 19);
	test(*s_arr[4], "gfsrt", 21, 19);
	test(*s_arr[45], "pfsocbhjtm", 21, 19);
	test(*s_arr[46], "tpflmdnoicjgkberhqsa", 21, S::npos);
}

template <class S>
void
test1(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(*s_arr[0], "", S::npos);
	test(*s_arr[0], "laenf", S::npos);
	test(*s_arr[0], "pqlnkmbdjo", S::npos);
	test(*s_arr[0], "qkamfogpnljdcshbreti", S::npos);
	test(*s_arr[58], "", 4);
	test(*s_arr[44], "irkhs", 4);
	test(*s_arr[28], "kantesmpgj", 4);
	test(*s_arr[61], "oknlrstdpiqmjbaghcfe", S::npos);
	test(*s_arr[19], "", 9);
	test(*s_arr[57], "bnrpe", 8);
	test(*s_arr[37], "jtdaefblso", 9);
	test(*s_arr[33], "oselktgbcapndfjihrmq", S::npos);
	test(*s_arr[30], "", 19);
	test(*s_arr[72], "bjaht", 18);
	test(*s_arr[68], "hjlcmgpket", 17);
	test(*s_arr[69], "htaobedqikfplcgjsmrn", S::npos);
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "string_test", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto &s_arr = pop.root()->s_arr;

	try {
		nvobj::transaction::run(pop, [&] {
			s_arr[0] = nvobj::make_persistent<C>("");
			s_arr[1] = nvobj::make_persistent<C>(
				"aemtbrgcklhndjisfpoq");
			s_arr[2] = nvobj::make_persistent<C>("aidjksrolc");
			s_arr[3] = nvobj::make_persistent<C>("akiteljmoh");
			s_arr[4] = nvobj::make_persistent<C>(
				"binjagtfldkrspcomqeh");
			s_arr[5] = nvobj::make_persistent<C>("brqgo");
			s_arr[6] = nvobj::make_persistent<C>("cdaih");
			s_arr[7] = nvobj::make_persistent<C>("clbao");
			s_arr[8] = nvobj::make_persistent<C>(
				"copqdhstbingamjfkler");
			s_arr[9] = nvobj::make_persistent<C>("cpebqsfmnj");
			s_arr[10] = nvobj::make_persistent<C>("crnklpmegd");
			s_arr[11] = nvobj::make_persistent<C>(
				"crsplifgtqedjohnabmk");
			s_arr[12] = nvobj::make_persistent<C>("cshmd");
			s_arr[13] = nvobj::make_persistent<C>("daiprenocl");
			s_arr[14] = nvobj::make_persistent<C>("dfsjhanorc");
			s_arr[15] = nvobj::make_persistent<C>(
				"dicfltehbsgrmojnpkaq");
			s_arr[16] = nvobj::make_persistent<C>("drtasbgmfp");
			s_arr[17] = nvobj::make_persistent<C>(
				"eaintpchlqsbdgrkjofm");
			s_arr[18] = nvobj::make_persistent<C>("elgofjmbrq");
			s_arr[19] = nvobj::make_persistent<C>("eolhfgpjqk");
			s_arr[20] = nvobj::make_persistent<C>("eqmpa");
			s_arr[21] = nvobj::make_persistent<C>("eqsgalomhb");
			s_arr[22] = nvobj::make_persistent<C>(
				"fkdrbqltsgmcoiphneaj");
			s_arr[23] = nvobj::make_persistent<C>("fmtsp");
			s_arr[24] = nvobj::make_persistent<C>("fodgq");
			s_arr[25] = nvobj::make_persistent<C>("gbmetiprqd");
			s_arr[26] = nvobj::make_persistent<C>("gfshlcmdjr");
			s_arr[27] = nvobj::make_persistent<C>(
				"gjnhidfsepkrtaqbmclo");
			s_arr[28] = nvobj::make_persistent<C>("gmfhd");
			s_arr[29] = nvobj::make_persistent<C>(
				"gpifsqlrdkbonjtmheca");
			s_arr[30] = nvobj::make_persistent<C>(
				"gprdcokbnjhlsfmtieqa");
			s_arr[31] = nvobj::make_persistent<C>("hcjitbfapl");
			s_arr[32] = nvobj::make_persistent<C>(
				"hdpkobnsalmcfijregtq");
			s_arr[33] = nvobj::make_persistent<C>("hkbgspoflt");
			s_arr[34] = nvobj::make_persistent<C>("hlbdfreqjo");
			s_arr[35] = nvobj::make_persistent<C>("hnefkqimca");
			s_arr[36] = nvobj::make_persistent<C>("igdsc");
			s_arr[37] = nvobj::make_persistent<C>("jdmciepkaq");
			s_arr[38] = nvobj::make_persistent<C>(
				"jlnkraeodhcspfgbqitm");
			s_arr[39] = nvobj::make_persistent<C>("jsbtafedoc");
			s_arr[40] = nvobj::make_persistent<C>(
				"jtlshdgqaiprkbcoenfm");
			s_arr[41] = nvobj::make_persistent<C>("khbpm");
			s_arr[42] = nvobj::make_persistent<C>(
				"kojatdhlcmigpbfrqnes");
			s_arr[43] = nvobj::make_persistent<C>("kthqnfcerm");
			s_arr[44] = nvobj::make_persistent<C>("lahfb");
			s_arr[45] = nvobj::make_persistent<C>(
				"latkmisecnorjbfhqpdg");
			s_arr[46] = nvobj::make_persistent<C>(
				"lecfratdjkhnsmqpoigb");
			s_arr[47] = nvobj::make_persistent<C>("lhcdo");
			s_arr[48] = nvobj::make_persistent<C>(
				"lhosrngtmfjikbqpcade");
			s_arr[49] = nvobj::make_persistent<C>(
				"liatsqdoegkmfcnbhrpj");
			s_arr[50] = nvobj::make_persistent<C>("litpcfdghe");
			s_arr[51] = nvobj::make_persistent<C>("lmofqdhpki");
			s_arr[52] = nvobj::make_persistent<C>("lsaijeqhtr");
			s_arr[53] = nvobj::make_persistent<C>("mekdn");
			s_arr[54] = nvobj::make_persistent<C>("mjqdgalkpc");
			s_arr[55] = nvobj::make_persistent<C>("mprdj");
			s_arr[56] = nvobj::make_persistent<C>(
				"mrtaefilpdsgocnhqbjk");
			s_arr[57] = nvobj::make_persistent<C>("nbatdlmekr");
			s_arr[58] = nvobj::make_persistent<C>("nhmko");
			s_arr[59] = nvobj::make_persistent<C>(
				"niptglfbosehkamrdqcj");
			s_arr[60] = nvobj::make_persistent<C>("nkodajteqp");
			s_arr[61] = nvobj::make_persistent<C>("odaft");
			s_arr[62] = nvobj::make_persistent<C>("oemth");
			s_arr[63] = nvobj::make_persistent<C>("omigs");
			s_arr[64] = nvobj::make_persistent<C>("onmje");
			s_arr[65] = nvobj::make_persistent<C>("oqmrj");
			s_arr[66] = nvobj::make_persistent<C>("pbsji");
			s_arr[67] = nvobj::make_persistent<C>("pkrof");
			s_arr[68] = nvobj::make_persistent<C>(
				"pnalfrdtkqcmojiesbhg");
			s_arr[69] = nvobj::make_persistent<C>(
				"pniotcfrhqsmgdkjbael");
			s_arr[70] = nvobj::make_persistent<C>(
				"pnracgfkjdiholtbqsem");
			s_arr[71] = nvobj::make_persistent<C>("qanej");
			s_arr[72] = nvobj::make_persistent<C>(
				"qjghlnftcaismkropdeb");
			s_arr[73] = nvobj::make_persistent<C>("qnmodrtkeb");
			s_arr[74] = nvobj::make_persistent<C>("qnsoh");
			s_arr[75] = nvobj::make_persistent<C>("qohtk");
			s_arr[76] = nvobj::make_persistent<C>("qpghtfbaji");
			s_arr[77] = nvobj::make_persistent<C>(
				"rbtaqjhgkneisldpmfoc");
			s_arr[78] = nvobj::make_persistent<C>(
				"rphmlekgfscndtaobiqj");
			s_arr[79] = nvobj::make_persistent<C>("schfa");
			s_arr[80] = nvobj::make_persistent<C>(
				"snafbdlghrjkpqtoceim");
			s_arr[81] = nvobj::make_persistent<C>(
				"spocfaktqdbiejlhngmr");
			s_arr[82] = nvobj::make_persistent<C>("srdfq");
			s_arr[83] = nvobj::make_persistent<C>("taqobhlerg");
			s_arr[84] = nvobj::make_persistent<C>("tnrph");
		});

		test0<C>(pop);
		test1<C>(pop);

		nvobj::transaction::run(pop, [&] {
			for (unsigned i = 0; i < 85; ++i) {
				nvobj::delete_persistent<C>(s_arr[i]);
			}
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
