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
	nvobj::persistent_ptr<C> s_arr[151];
};

template <class S>
void
test(const S &s, const S &str, typename S::size_type pos,
     typename S::size_type x)
{
	UT_ASSERT(s.find_first_not_of(str, pos) == x);
	if (x != S::npos)
		UT_ASSERT(pos <= x && x < s.size());
}

template <class S>
void
test(const S &s, const S &str, typename S::size_type x)
{
	UT_ASSERT(s.find_first_not_of(str) == x);
	if (x != S::npos)
		UT_ASSERT(x < s.size());
}

template <class S>
void
test0(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(*s_arr[0], *s_arr[0], 0, S::npos);
	test(*s_arr[0], *s_arr[78], 0, S::npos);
	test(*s_arr[0], *s_arr[126], 0, S::npos);
	test(*s_arr[0], *s_arr[130], 0, S::npos);
	test(*s_arr[0], *s_arr[0], 1, S::npos);
	test(*s_arr[0], *s_arr[13], 1, S::npos);
	test(*s_arr[0], *s_arr[54], 1, S::npos);
	test(*s_arr[0], *s_arr[59], 1, S::npos);
	test(*s_arr[43], *s_arr[0], 0, 0);
	test(*s_arr[128], *s_arr[26], 0, 0);
	test(*s_arr[19], *s_arr[61], 0, 0);
	test(*s_arr[92], *s_arr[100], 0, S::npos);
	test(*s_arr[145], *s_arr[0], 1, 1);
	test(*s_arr[108], *s_arr[62], 1, 1);
	test(*s_arr[18], *s_arr[31], 1, 3);
	test(*s_arr[134], *s_arr[113], 1, S::npos);
	test(*s_arr[24], *s_arr[0], 2, 2);
	test(*s_arr[84], *s_arr[107], 2, 2);
	test(*s_arr[133], *s_arr[75], 2, S::npos);
	test(*s_arr[122], *s_arr[1], 2, S::npos);
	test(*s_arr[42], *s_arr[0], 4, 4);
	test(*s_arr[72], *s_arr[5], 4, 4);
	test(*s_arr[117], *s_arr[118], 4, 4);
	test(*s_arr[96], *s_arr[40], 4, S::npos);
	test(*s_arr[37], *s_arr[0], 5, S::npos);
	test(*s_arr[111], *s_arr[73], 5, S::npos);
	test(*s_arr[112], *s_arr[39], 5, S::npos);
	test(*s_arr[114], *s_arr[66], 5, S::npos);
	test(*s_arr[141], *s_arr[0], 6, S::npos);
	test(*s_arr[60], *s_arr[131], 6, S::npos);
	test(*s_arr[17], *s_arr[137], 6, S::npos);
	test(*s_arr[149], *s_arr[148], 6, S::npos);
	test(*s_arr[52], *s_arr[0], 0, 0);
	test(*s_arr[25], *s_arr[8], 0, 2);
	test(*s_arr[87], *s_arr[93], 0, 1);
	test(*s_arr[3], *s_arr[63], 0, S::npos);
	test(*s_arr[135], *s_arr[0], 1, 1);
	test(*s_arr[45], *s_arr[98], 1, 1);
	test(*s_arr[103], *s_arr[109], 1, 4);
	test(*s_arr[44], *s_arr[10], 1, S::npos);
	test(*s_arr[22], *s_arr[0], 5, 5);
	test(*s_arr[68], *s_arr[127], 5, 5);
	test(*s_arr[132], *s_arr[120], 5, 6);
	test(*s_arr[21], *s_arr[106], 5, S::npos);
	test(*s_arr[88], *s_arr[0], 9, 9);
	test(*s_arr[57], *s_arr[140], 9, S::npos);
	test(*s_arr[33], *s_arr[77], 9, 9);
	test(*s_arr[90], *s_arr[139], 9, S::npos);
	test(*s_arr[35], *s_arr[0], 10, S::npos);
	test(*s_arr[94], *s_arr[32], 10, S::npos);
	test(*s_arr[76], *s_arr[30], 10, S::npos);
	test(*s_arr[27], *s_arr[58], 10, S::npos);
	test(*s_arr[38], *s_arr[0], 11, S::npos);
	test(*s_arr[4], *s_arr[89], 11, S::npos);
	test(*s_arr[56], *s_arr[9], 11, S::npos);
	test(*s_arr[146], *s_arr[119], 11, S::npos);
	test(*s_arr[143], *s_arr[0], 0, 0);
	test(*s_arr[2], *s_arr[81], 0, 0);
	test(*s_arr[125], *s_arr[147], 0, 1);
	test(*s_arr[28], *s_arr[142], 0, S::npos);
	test(*s_arr[67], *s_arr[0], 1, 1);
	test(*s_arr[85], *s_arr[6], 1, 1);
	test(*s_arr[136], *s_arr[51], 1, 3);
	test(*s_arr[49], *s_arr[95], 1, S::npos);
	test(*s_arr[53], *s_arr[0], 10, 10);
	test(*s_arr[70], *s_arr[116], 10, 11);
	test(*s_arr[41], *s_arr[7], 10, 13);
	test(*s_arr[23], *s_arr[14], 10, S::npos);
	test(*s_arr[102], *s_arr[0], 19, 19);
	test(*s_arr[20], *s_arr[29], 19, 19);
	test(*s_arr[97], *s_arr[83], 19, S::npos);
	test(*s_arr[74], *s_arr[16], 19, S::npos);
	test(*s_arr[34], *s_arr[0], 20, S::npos);
	test(*s_arr[47], *s_arr[104], 20, S::npos);
	test(*s_arr[144], *s_arr[11], 20, S::npos);
	test(*s_arr[138], *s_arr[91], 20, S::npos);
	test(*s_arr[86], *s_arr[0], 21, S::npos);
	test(*s_arr[12], *s_arr[46], 21, S::npos);
	test(*s_arr[80], *s_arr[121], 21, S::npos);
	test(*s_arr[82], *s_arr[150], 21, S::npos);
}

template <class S>
void
test1(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(*s_arr[0], *s_arr[0], S::npos);
	test(*s_arr[0], *s_arr[78], S::npos);
	test(*s_arr[0], *s_arr[126], S::npos);
	test(*s_arr[0], *s_arr[130], S::npos);
	test(*s_arr[101], *s_arr[0], 0);
	test(*s_arr[79], *s_arr[64], 0);
	test(*s_arr[48], *s_arr[71], 2);
	test(*s_arr[105], *s_arr[110], S::npos);
	test(*s_arr[36], *s_arr[0], 0);
	test(*s_arr[99], *s_arr[15], 2);
	test(*s_arr[65], *s_arr[69], 2);
	test(*s_arr[55], *s_arr[115], S::npos);
	test(*s_arr[50], *s_arr[0], 0);
	test(*s_arr[129], *s_arr[13], 0);
	test(*s_arr[123], *s_arr[54], 1);
	test(*s_arr[124], *s_arr[59], S::npos);
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
				"acbsjqogpltdkhinfrem");
			s_arr[2] = nvobj::make_persistent<C>(
				"aemtbrgcklhndjisfpoq");
			s_arr[3] = nvobj::make_persistent<C>("aidjksrolc");
			s_arr[4] = nvobj::make_persistent<C>("akiteljmoh");
			s_arr[5] = nvobj::make_persistent<C>("aobjd");
			s_arr[6] = nvobj::make_persistent<C>("aqibs");
			s_arr[7] = nvobj::make_persistent<C>("arosdhcfme");
			s_arr[8] = nvobj::make_persistent<C>("ashjd");
			s_arr[9] = nvobj::make_persistent<C>("astoegbfpn");
			s_arr[10] = nvobj::make_persistent<C>(
				"bdfjqgatlksriohemnpc");
			s_arr[11] = nvobj::make_persistent<C>("bgtajmiedc");
			s_arr[12] = nvobj::make_persistent<C>(
				"binjagtfldkrspcomqeh");
			s_arr[13] = nvobj::make_persistent<C>("bjaht");
			s_arr[14] = nvobj::make_persistent<C>(
				"blkhjeogicatqfnpdmsr");
			s_arr[15] = nvobj::make_persistent<C>("bnrpe");
			s_arr[16] = nvobj::make_persistent<C>(
				"bqjhtkfepimcnsgrlado");
			s_arr[17] = nvobj::make_persistent<C>("brqgo");
			s_arr[18] = nvobj::make_persistent<C>("cdaih");
			s_arr[19] = nvobj::make_persistent<C>("clbao");
			s_arr[20] = nvobj::make_persistent<C>(
				"copqdhstbingamjfkler");
			s_arr[21] = nvobj::make_persistent<C>("cpebqsfmnj");
			s_arr[22] = nvobj::make_persistent<C>("crnklpmegd");
			s_arr[23] = nvobj::make_persistent<C>(
				"crsplifgtqedjohnabmk");
			s_arr[24] = nvobj::make_persistent<C>("cshmd");
			s_arr[25] = nvobj::make_persistent<C>("daiprenocl");
			s_arr[26] = nvobj::make_persistent<C>("dfkap");
			s_arr[27] = nvobj::make_persistent<C>("dfsjhanorc");
			s_arr[28] = nvobj::make_persistent<C>(
				"dicfltehbsgrmojnpkaq");
			s_arr[29] = nvobj::make_persistent<C>("djkqc");
			s_arr[30] = nvobj::make_persistent<C>("dkacjoptns");
			s_arr[31] = nvobj::make_persistent<C>("dmajblfhsg");
			s_arr[32] = nvobj::make_persistent<C>("dplqa");
			s_arr[33] = nvobj::make_persistent<C>("drtasbgmfp");
			s_arr[34] = nvobj::make_persistent<C>(
				"eaintpchlqsbdgrkjofm");
			s_arr[35] = nvobj::make_persistent<C>("elgofjmbrq");
			s_arr[36] = nvobj::make_persistent<C>("eolhfgpjqk");
			s_arr[37] = nvobj::make_persistent<C>("eqmpa");
			s_arr[38] = nvobj::make_persistent<C>("eqsgalomhb");
			s_arr[39] = nvobj::make_persistent<C>("fbslrjiqkm");
			s_arr[40] = nvobj::make_persistent<C>(
				"fhepcrntkoagbmldqijs");
			s_arr[41] = nvobj::make_persistent<C>(
				"fkdrbqltsgmcoiphneaj");
			s_arr[42] = nvobj::make_persistent<C>("fmtsp");
			s_arr[43] = nvobj::make_persistent<C>("fodgq");
			s_arr[44] = nvobj::make_persistent<C>("gbmetiprqd");
			s_arr[45] = nvobj::make_persistent<C>("gfshlcmdjr");
			s_arr[46] = nvobj::make_persistent<C>("gfsrt");
			s_arr[47] = nvobj::make_persistent<C>(
				"gjnhidfsepkrtaqbmclo");
			s_arr[48] = nvobj::make_persistent<C>("gmfhd");
			s_arr[49] = nvobj::make_persistent<C>(
				"gpifsqlrdkbonjtmheca");
			s_arr[50] = nvobj::make_persistent<C>(
				"gprdcokbnjhlsfmtieqa");
			s_arr[51] = nvobj::make_persistent<C>("gtfblmqinc");
			s_arr[52] = nvobj::make_persistent<C>("hcjitbfapl");
			s_arr[53] = nvobj::make_persistent<C>(
				"hdpkobnsalmcfijregtq");
			s_arr[54] = nvobj::make_persistent<C>("hjlcmgpket");
			s_arr[55] = nvobj::make_persistent<C>("hkbgspoflt");
			s_arr[56] = nvobj::make_persistent<C>("hlbdfreqjo");
			s_arr[57] = nvobj::make_persistent<C>("hnefkqimca");
			s_arr[58] = nvobj::make_persistent<C>(
				"hqfimtrgnbekpdcsjalo");
			s_arr[59] = nvobj::make_persistent<C>(
				"htaobedqikfplcgjsmrn");
			s_arr[60] = nvobj::make_persistent<C>("igdsc");
			s_arr[61] = nvobj::make_persistent<C>("ihqrfebgad");
			s_arr[62] = nvobj::make_persistent<C>("ikcrq");
			s_arr[63] = nvobj::make_persistent<C>(
				"imqnaghkfrdtlopbjesc");
			s_arr[64] = nvobj::make_persistent<C>("irkhs");
			s_arr[65] = nvobj::make_persistent<C>("jdmciepkaq");
			s_arr[66] = nvobj::make_persistent<C>(
				"jeidpcmalhfnqbgtrsko");
			s_arr[67] = nvobj::make_persistent<C>(
				"jlnkraeodhcspfgbqitm");
			s_arr[68] = nvobj::make_persistent<C>("jsbtafedoc");
			s_arr[69] = nvobj::make_persistent<C>("jtdaefblso");
			s_arr[70] = nvobj::make_persistent<C>(
				"jtlshdgqaiprkbcoenfm");
			s_arr[71] = nvobj::make_persistent<C>("kantesmpgj");
			s_arr[72] = nvobj::make_persistent<C>("khbpm");
			s_arr[73] = nvobj::make_persistent<C>("kocgb");
			s_arr[74] = nvobj::make_persistent<C>(
				"kojatdhlcmigpbfrqnes");
			s_arr[75] = nvobj::make_persistent<C>("kojhpmbsfe");
			s_arr[76] = nvobj::make_persistent<C>("kthqnfcerm");
			s_arr[77] = nvobj::make_persistent<C>("ktsrmnqagd");
			s_arr[78] = nvobj::make_persistent<C>("laenf");
			s_arr[79] = nvobj::make_persistent<C>("lahfb");
			s_arr[80] = nvobj::make_persistent<C>(
				"latkmisecnorjbfhqpdg");
			s_arr[81] = nvobj::make_persistent<C>("lbtqd");
			s_arr[82] = nvobj::make_persistent<C>(
				"lecfratdjkhnsmqpoigb");
			s_arr[83] = nvobj::make_persistent<C>("lgokshjtpb");
			s_arr[84] = nvobj::make_persistent<C>("lhcdo");
			s_arr[85] = nvobj::make_persistent<C>(
				"lhosrngtmfjikbqpcade");
			s_arr[86] = nvobj::make_persistent<C>(
				"liatsqdoegkmfcnbhrpj");
			s_arr[87] = nvobj::make_persistent<C>("litpcfdghe");
			s_arr[88] = nvobj::make_persistent<C>("lmofqdhpki");
			s_arr[89] = nvobj::make_persistent<C>("lofbc");
			s_arr[90] = nvobj::make_persistent<C>("lsaijeqhtr");
			s_arr[91] = nvobj::make_persistent<C>(
				"lsckfnqgdahejiopbtmr");
			s_arr[92] = nvobj::make_persistent<C>("mekdn");
			s_arr[93] = nvobj::make_persistent<C>("mgojkldsqh");
			s_arr[94] = nvobj::make_persistent<C>("mjqdgalkpc");
			s_arr[95] = nvobj::make_persistent<C>(
				"mkqpbtdalgniorhfescj");
			s_arr[96] = nvobj::make_persistent<C>("mprdj");
			s_arr[97] = nvobj::make_persistent<C>(
				"mrtaefilpdsgocnhqbjk");
			s_arr[98] = nvobj::make_persistent<C>("nadkh");
			s_arr[99] = nvobj::make_persistent<C>("nbatdlmekr");
			s_arr[100] = nvobj::make_persistent<C>(
				"ngtjfcalbseiqrphmkdo");
			s_arr[101] = nvobj::make_persistent<C>("nhmko");
			s_arr[102] = nvobj::make_persistent<C>(
				"niptglfbosehkamrdqcj");
			s_arr[103] = nvobj::make_persistent<C>("nkodajteqp");
			s_arr[104] = nvobj::make_persistent<C>("nocfa");
			s_arr[105] = nvobj::make_persistent<C>("odaft");
			s_arr[106] = nvobj::make_persistent<C>(
				"odnqkgijrhabfmcestlp");
			s_arr[107] = nvobj::make_persistent<C>("oebqi");
			s_arr[108] = nvobj::make_persistent<C>("oemth");
			s_arr[109] = nvobj::make_persistent<C>("ofdrqmkebl");
			s_arr[110] = nvobj::make_persistent<C>(
				"oknlrstdpiqmjbaghcfe");
			s_arr[111] = nvobj::make_persistent<C>("omigs");
			s_arr[112] = nvobj::make_persistent<C>("onmje");
			s_arr[113] = nvobj::make_persistent<C>(
				"oqftjhdmkgsblacenirp");
			s_arr[114] = nvobj::make_persistent<C>("oqmrj");
			s_arr[115] = nvobj::make_persistent<C>(
				"oselktgbcapndfjihrmq");
			s_arr[116] = nvobj::make_persistent<C>("pblas");
			s_arr[117] = nvobj::make_persistent<C>("pbsji");
			s_arr[118] = nvobj::make_persistent<C>("pcbahntsje");
			s_arr[119] = nvobj::make_persistent<C>(
				"pdgreqomsncafklhtibj");
			s_arr[120] = nvobj::make_persistent<C>("pejafmnokr");
			s_arr[121] = nvobj::make_persistent<C>("pfsocbhjtm");
			s_arr[122] = nvobj::make_persistent<C>("pkrof");
			s_arr[123] = nvobj::make_persistent<C>(
				"pnalfrdtkqcmojiesbhg");
			s_arr[124] = nvobj::make_persistent<C>(
				"pniotcfrhqsmgdkjbael");
			s_arr[125] = nvobj::make_persistent<C>(
				"pnracgfkjdiholtbqsem");
			s_arr[126] = nvobj::make_persistent<C>("pqlnkmbdjo");
			s_arr[127] = nvobj::make_persistent<C>("prqgn");
			s_arr[128] = nvobj::make_persistent<C>("qanej");
			s_arr[129] = nvobj::make_persistent<C>(
				"qjghlnftcaismkropdeb");
			s_arr[130] = nvobj::make_persistent<C>(
				"qkamfogpnljdcshbreti");
			s_arr[131] = nvobj::make_persistent<C>("qngpd");
			s_arr[132] = nvobj::make_persistent<C>("qnmodrtkeb");
			s_arr[133] = nvobj::make_persistent<C>("qnsoh");
			s_arr[134] = nvobj::make_persistent<C>("qohtk");
			s_arr[135] = nvobj::make_persistent<C>("qpghtfbaji");
			s_arr[136] = nvobj::make_persistent<C>(
				"rbtaqjhgkneisldpmfoc");
			s_arr[137] = nvobj::make_persistent<C>("rodhqklgmb");
			s_arr[138] = nvobj::make_persistent<C>(
				"rphmlekgfscndtaobiqj");
			s_arr[139] = nvobj::make_persistent<C>(
				"rtdhgcisbnmoaqkfpjle");
			s_arr[140] = nvobj::make_persistent<C>("rtjpa");
			s_arr[141] = nvobj::make_persistent<C>("schfa");
			s_arr[142] = nvobj::make_persistent<C>(
				"slcerthdaiqjfnobgkpm");
			s_arr[143] = nvobj::make_persistent<C>(
				"snafbdlghrjkpqtoceim");
			s_arr[144] = nvobj::make_persistent<C>(
				"spocfaktqdbiejlhngmr");
			s_arr[145] = nvobj::make_persistent<C>("srdfq");
			s_arr[146] = nvobj::make_persistent<C>("taqobhlerg");
			s_arr[147] = nvobj::make_persistent<C>("tboimldpjh");
			s_arr[148] = nvobj::make_persistent<C>(
				"thdjgafrlbkoiqcspmne");
			s_arr[149] = nvobj::make_persistent<C>("tnrph");
			s_arr[150] = nvobj::make_persistent<C>(
				"tpflmdnoicjgkberhqsa");
		});

		test0<C>(pop);
		test1<C>(pop);

		nvobj::transaction::run(pop, [&] {
			for (unsigned i = 0; i < 151; ++i) {
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
