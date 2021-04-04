// GuiNN Checkers 2.0
// by Jonathan Kreuzer
// with contributions by Ed Gilbert and Ed Trice
// copyright 2005, 2020
// http://www.3dkingdoms.com/checkers.htm
//
// NOTICES & CONDITIONS
//
// Commercial use of this code, in whole or in any part, is prohibited.
// Non-commercial use is welcome, but you should clearly mention this code, and where it came from.
// Also the source should remain publicly available (if you distribute your modified program.)
// www.3dkingdoms.com/checkers.htm
//
// These conditions apply to the code itself only.
// You are of course welcome to read this code and learn from it without restrictions

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <memory.h>
#include <assert.h>
#include <cstdint>
#include <initializer_list>
#include <algorithm>

#include "defines.h"
#include "resource.h"
#include "cb_interface.h"
#include "registry.h"
#include "engine.h"
#include "learning.h"
#include "guiWindows.h"
#include "checkersGui.h" //  TODO : avoid including specific gui?

// Our Checkers Engine
Engine engine;

// For external Checkerboard interface
CheckerboardInterface checkerBoard;

// Transposition table hash function values
uint64_t TranspositionTable::HashSTM;
uint64_t TranspositionTable::HashFunction[NUM_BOARD_SQUARES][NUM_PIECE_TYPES] =
{
	{ 0x00713BBE67CF1E6C, 0x2D4917525FA6B6F1, 0x5B32E1E901F6E1A6, 0x12F0BA87391B3E99, 0x0154A20D496953B7,
	0x159C17B32D199AC8, 0x64A9FD8B27167C03, 0x7AD0C2091273431F, 0x6E7837CB6C7C15F5, },
	{ 0x4E775D1326788C0A, 0x302831AE0733959A, 0x2373394058E392FD, 0x3E2CA8323C31177D, 0x5F5728AD31AD61F2,
	0x497253661D06D1C4, 0x42AF17322C50F522, 0x3EFE7B9140B05A8B, 0x129332B02700A202, },
	{ 0x7C10859D70B27680, 0x18952D993CE92980, 0x5DE60DC94923B3BF, 0x5CA3E5BF2F7EDA7E, 0x4282128E0DDFF8EF,
	0x46838561306B918F, 0x3A844AD1267CE59C, 0x1977DD7233268EF0, 0x0403984A06BB1AD7, },
	{ 0x6C388D2C19BEDAC9, 0x0E7146337898C90C, 0x12523FD45AEC33A4, 0x206B7E3508369F22, 0x1B0312CF01E1CA2D,
	0x60A0188F09ACFFD9, 0x594261E51E0A3B2B, 0x508CC1877B9D845F, 0x187B16D47F9C0CBE, },
	{ 0x0CCB1115387E4133, 0x72A73C1862A4ABDA, 0x50B18DCE3BFDD63E, 0x6DD364F85C8E9A04, 0x17F4BA0E73F82E2F,
	0x4DC0B2D42D36544A, 0x5F1EA66858DD0D16, 0x4A2A85AD4F0B2988, 0x55F6265249023B43, },
	{ 0x0E54A8F45F58FCD8, 0x0A32200B693AFE60, 0x347F7897403AB359, 0x5E9E96D07B3225CD, 0x26E0E44908CEBC25,
	0x4E826A407067DA3B, 0x0DAAC211259E078C, 0x54E4E80B30C6FC8C, 0x41604915446ACC95, },
	{ 0x011BA7E1502EBCE9, 0x5FE7CAA764CCEFB5, 0x7AA4D59A2FF83823, 0x7A1F58902A3F79D1, 0x1145DAA14CC2CB4E,
	0x01F18A9C4F19E171, 0x15EF0DA52911609B, 0x671B8A654E9E9D46, 0x12D9DC82208AE776, },
	{ 0x08AF0B6326F742DF, 0x27E8906D1162C6E0, 0x72FB71344EEDA6A6, 0x1DADA65E2036EB0E, 0x06EDAF2836AD3F20,
	0x723ADDC2107A4941, 0x060EAF0F75473C65, 0x79866C616C21F7E7, 0x212CD1467A6926DC, },
	{ 0x4B13FF8D79362FF2, 0x0163BDFF64E9E23B, 0x1CA6068C3588DB78, 0x6D7BB4D05516E8BE, 0x4432D446589E8E8C,
	0x0469827F5A7B36A7, 0x795BA9321312F95B, 0x25DE420B59020FF5, 0x1F0D317D0E12C18A, },
	{ 0x0AA530FD7D1BBE68, 0x1B30727B6022CCCD, 0x285211F0205C59B4, 0x13F5D753784C6ECB, 0x52F5845E3AC24103,
	0x139DBBE6367DF931, 0x4A52CAAD32FBD9B0, 0x65E32FFF3223A614, 0x196146793A74B242, },
	{ 0x38D3CEC16B43E68C, 0x79F752237E7C2AA6, 0x49BFE54C5AB67E8D, 0x21829F995B44AFB1, 0x329351B127B1F608,
	0x2D9F48675E9FB6D5, 0x19C23B1874CE5742, 0x0E743FFE42A00921, 0x580F9D2F09B6BC73, },
	{ 0x27AC52433293057E, 0x5CE3C7D54F2318A1, 0x0CDAD45A7CF9E127, 0x5D6E4D0758DCD2BE, 0x06CCBBB407F12469,
	0x1B5B720901648E01, 0x4DF04A562D32539D, 0x5047ACD84879B981, 0x275C646B4E9A1462, },
	{ 0x0433C1D0781ACE17, 0x4EAFE80D4ADE331D, 0x65C174994BBB4374, 0x5647FB4F2262D5B5, 0x62E004570F522F80,
	0x7283B18D73D8548B, 0x19B6CB117A7516F6, 0x50910BF7220B72B8, 0x64F63C603D99D53C, },
	{ 0x6D8B1B5B499EEB8A, 0x18ECA65C024A3D87, 0x16535E325706A840, 0x2FE7E3C351CEC130, 0x7C6E12EE62887503,
	0x015BF06A2C244550, 0x0BE121812B979928, 0x44D2FCE93BBF7DA1, 0x6EBF533148C730A4, },
	{ 0x6F0B03816EF5EB98, 0x6A726E9A74B80FCF, 0x3864F450698FD4BD, 0x00708A030833B05D, 0x57F072B9124F604E,
	0x02DD8EE507535398, 0x3744D10D327F1A14, 0x0355CCB85175F0E2, 0x3B6CA47D64491584, },
	{ 0x4DE09D0160D18A6B, 0x5B9639435270FC19, 0x583A60982571872E, 0x1FD1380E7A321DF0, 0x5003743D5FEBAE31,
	0x5106BFAF75CDF3A4, 0x3A024D5E178A59FC, 0x5DF8C602059D1B6B, 0x477BEA21085A8D5A, },
	{ 0x4CF0881E54ACB571, 0x53DCC2B81CB4EE4E, 0x3FF060BF37B0BF74, 0x302DDEE55E4E6258, 0x538837F25ED5BCA0,
	0x2E8DCFCB695743E9, 0x6FAA790235CCDAAC, 0x153519F74BB87523, 0x4A98B8A37588DF35, },
	{ 0x67B8E7AD62FB3FD5, 0x31C97C5D1984FD5A, 0x3102F87719634DB5, 0x61D9C53A39A2F998, 0x3B4CB6EB1ECEA5DF,
	0x7676D5B207E125E6, 0x6004E6BC74DC788D, 0x46C9D1D665842414, 0x5DF0490470772818, },
	{ 0x32CAB4BB53737E60, 0x60E633CF07B2DA2F, 0x2C9EB9293854F959, 0x26097CEA0A32FF55, 0x12C3A784304130F8,
	0x68562AED20F794A0, 0x1DC8BDFA720B6EFC, 0x05524D852DC798B0, 0x72CD04EE74F44185, },
	{ 0x806581076B9DF5EC, 0x247C5EA35F4EBCF4, 0x52F8643268C95004, 0x3055A22C71E48101, 0x3975154335A480A2,
	0x405676927423571C, 0x62D27E3E67A43872, 0x457A614652E98E09, 0x2FACFA7A480AFC55, },
	{ 0x2A0A2BCA78676EB6, 0x72A5246E5186FFE0, 0x363C8A9B12015F5F, 0x578857BA4E1E1A1B, 0x691EB761421F92E5,
	0x5FF32F783B8D5077, 0x50C66F597C4A686A, 0x227394C44C932A0F, 0x3B42F00102184F30, },
	{ 0x33E31D892544D622, 0x37C86D05443EC6B3, 0x6EF6568D2AEB58D0, 0x47A62E691E4C6804, 0x7BB537BF09FD4014,
	0x3FDBAEF827567F68, 0x4B6CD0C048350C02, 0x26F180EC1DACA230, 0x723F86AF7F6AE441, },
	{ 0x20F8CBDD1DE0D2DA, 0x06595C3824103BCA, 0x353BA8EF25A21869, 0x7A0A9ADE6615E7E2, 0x6F872E3622C41A00,
	0x10B96B8716876F54, 0x3AA4706C01F5A4C5, 0x666E1F4926A17AF3, 0x6BE94DD702FBB4F4, },
	{ 0x2DF3C374173E394F, 0x5F69467443755642, 0x14C73EDF03D5A1B6, 0x202F386745718A74, 0x77D50B7713E4D16E,
	0x729CB7A76975F93E, 0x563BDC047A051330, 0x4C44EDE419A31B61, 0x2D98AB3D5905B1E7, },
	{ 0x7DE2A8BC1EA22FFF, 0x2A0E150F2B3FA5A9, 0x2540871E5F149E41, 0x5807F428089BCFF5, 0x2BBDEBFD0314982B,
	0x457E99CA63D2452E, 0x61B42E393699ACBE, 0x3821A5CE227FDC57, 0x7DF421C90946844E, },
	{ 0x6E4150F625160510, 0x161BEE2D26E90A26, 0x0AD7B3781C2018D1, 0x50FBE913614BC159, 0x24824DC649153465,
	0x664F45474F826CEB, 0x195A68A20588B9A8, 0x3477E2E07364EE20, 0x1A7514622D7D9284, },
	{ 0x7D0B12A82CE36122, 0x19F4517F443D5B28, 0x0CCE03CB6C5767D9, 0x00FC316477DD808B, 0x600A4DC36A8DB506,
	0x64747C7458650220, 0x0D40B283581E38A2, 0x1D5C9F5C61B955DA, 0x07AB13E148DC7D75, },
	{ 0x70AB839A3541B363, 0x791F3D494EE7A7F9, 0x367C794E19EB3597, 0x5029799F0DAB9F70, 0x28F5ED3F27F99547,
	0x3C8996742F5FCB43, 0x4ABB843032D3FC2B, 0x35E23F6C0AC8749D, 0x0B56D86450B1A227, },
	{ 0x4EBA6BEE69D0B248, 0x1C55FF6E21DF3AC2, 0x62AF3F0E2FBCF7C7, 0x1CA6F93E27F76CE2, 0x6BEB04B75D0F1CB3,
	0x0AD6D06951D144CD, 0x0C4A401C2C60065A, 0x27CBEAB71777F45C, 0x163DF26040B9C095, },
	{ 0x3999F2F23DEBC680, 0x204A088326C0ADE9, 0x5105C4EC218C88FA, 0x6E44958F214022E1, 0x0C62C2D1245E7456,
	0x42254E037CD48DAA, 0x754C11AE18123B29, 0x5709EB7F6B4AFFEF, 0x6F7D8E424505478B, },
	{ 0x1A4A67B11A04C2F7, 0x5EB60504651563D6, 0x6C33337F085FC0FC, 0x72737B743101C49F, 0x71CADE75567C6DCD,
	0x43F70EA55FC5B4F4, 0x47B1DB70076304A6, 0x2F4E400A2991DBD8, 0x200CC28B70563A4E, },
	{ 0x196C328137C8DDFB, 0x167DC8495AE6B772, 0x066D259D10C9D1BE, 0x491755040A080540, 0x34B53D37694B4165,
	0x7EE95F9524C2B947, 0x25E6603668143C9E, 0x6A6A034441B2888E, 0x3CE3399D466F3232, },
};

// -----------------------
// Display search information
// -----------------------
const char* GetNodeCount(uint64_t count, int nBuf)
{
	static char buffer[4][100];

	if (count < 1000)
		snprintf(buffer[nBuf], sizeof(buffer[nBuf]), "%d n", (int)count);
	else if (count < 1000000)
		snprintf(buffer[nBuf], sizeof(buffer[nBuf]), "%.2f kn", (float)count / 1000.0f);
	else
		snprintf(buffer[nBuf], sizeof(buffer[nBuf]), "%.2f Mn", (float)count / 1000000.0f);

	return buffer[nBuf];
}

void RunningDisplay(SearchInfo& displayInfo, const Move& bestMove, bool bSearching)
{
	char sTemp[4096];
	static int LastEval = 0;

	static Move LastBest = NO_MOVE;
	if (bestMove != NO_MOVE) {
		LastBest = bestMove;
	}

	int j = 0;
	if (!checkerBoard.bActive) {
		j += sprintf(sTemp + j, "Red: %d   White: %d                           ", engine.board.numPieces[BLACK], engine.board.numPieces[WHITE]);
		j += sprintf(sTemp + j, "Limits: %d-ply   %ds  ", engine.searchLimits.maxDepth, (int)engine.searchLimits.maxSeconds);
		j += sprintf(sTemp + j, "%s\n", bSearching ? "(searching...)" : "");
	}

	float seconds = TimeSince( displayInfo.startTimeMs );
	int nps = 0;
	if (seconds > 0.0f)
		nps = int(float(displayInfo.nodes) / (1000.0 * seconds));

	if (abs(displayInfo.eval) < 3000)
		LastEval = displayInfo.eval;

	// Show smiley if winning to better follow fast blitz matches
	bool winning = (-LastEval > 40 && engine.computerColor == BLACK) || (-LastEval < -40 && engine.computerColor == WHITE);
	winning = winning && displayInfo.depth > 6; // single move can create false moods because of low search depth
	const char* sMood = (winning && abs(LastEval) > 200) ? ":-D  " : winning ? ":-)  " : "";

	j += sprintf(sTemp + j,
		"%sEval: %d  Depth: %d/%d (%d/%d)  ",
		sMood,
		-LastEval,
		displayInfo.depth,
		displayInfo.selectiveDepth,
		displayInfo.searchingMove,
		displayInfo.numMoves );

	// Also announce distance to win when the databases are returning high scores
	if (abs(LastEval) > MIN_WIN_SCORE)
	{
		if (LastEval < 0) {
			j += sprintf(sTemp + j, "(Red wins in %d)", abs(2001 - abs(LastEval)));
		} else {
			j += sprintf(sTemp + j, "(White wins in %d)", abs(2001 - LastEval));
		}
	}

	if (!checkerBoard.bActive)
		j += sprintf(sTemp + j, "\n");

	j += sprintf(sTemp + j,
		"Move: %s   Time: %.2fs   Speed %d KN/s   Nodes: %s (db: %s) ",
		Transcript::GetMoveString(LastBest).c_str(),
		seconds,
		nps,
		GetNodeCount(displayInfo.nodes, 0),
		GetNodeCount(displayInfo.databaseNodes, 1) );

	if (!checkerBoard.bActive)
		j += sprintf(sTemp + j, "\n");

	j += sprintf(sTemp + j, "%s", displayInfo.pv.ToString().c_str());

	DisplayText(sTemp);
}

void Engine::NewGame( const Board& startBoard, bool resetTranscript )
{
	board = startBoard;
	searchThreadData.historyTable.Clear();

	if (resetTranscript) { 
		transcript.Init(startBoard); 
	} else {
		transcript.ReplayGame(board, boardHashHistory);
	}
}

std::string Engine::GetInfoString()
{
	// Endgame Database
	std::string displayStr;
	if (dbInfo.loaded == false)
	{
		displayStr = "No Database Loaded\n";
	} else {
		displayStr = "Database : " + std::to_string(dbInfo.numPieces) + "-piece ",
		displayStr += (dbInfo.type == dbType::WIN_LOSS_DRAW) || (dbInfo.type == dbType::KR_WIN_LOSS_DRAW) ? "Win/Loss/Draw\n" : "Perfect Play\n";
	}

	// Opening Book
	if (openingBook) {
		displayStr += "Opening Book : " + std::to_string(openingBook->m_nPositions) + " positions\n";
	} else {
		displayStr += "Opening Book : not loaded.\n";
	}

	// Neural Nets
	int numLoadedNets = 0;
	for (auto net : evalNets) { numLoadedNets += (net->isLoaded) ? 1 : 0; }
	displayStr += "Neural Nets : " + std::to_string(numLoadedNets) + "\n";

	return displayStr;
}

// TODO : convert to std::thread
HANDLE hEngineReady, hAction;
HANDLE hThread;

DWORD WINAPI ThinkingThread(void* /* param */)
{
	hEngineReady = CreateEvent(NULL, TRUE, FALSE, NULL);

	// Think of Move
	while (true)
	{
		SetEvent(hEngineReady);
		WaitForSingleObject(hAction, INFINITE);
		ResetEvent(hEngineReady);

		GUI.SetComputerColor((eColor)engine.board.sideToMove);
		BestMoveInfo moveInfo = ComputerMove( engine.board, engine.searchThreadData);
		GUI.DoGameMove(moveInfo.move);
		GUI.ThinkingMenuActive(false);
		engine.bThinking = false;
	}

	CloseHandle(hThread);
}

//
// ENGINE INITILIZATION
//
// status_str is NULL if not called from CheckerBoard
//
void Engine::Init(char* status_str)
{
	InitBitTables();
	InitializeNeuralNets();
	TranspositionTable::CreateHashFunction();
	openingBook = new COpeningBook;

	searchThreadData.historyTable.Clear();

	int firstLayerOutputCount = 0;
	for (auto net : evalNets)
		firstLayerOutputCount = std::max(firstLayerOutputCount, net->network.GetLayer(0)->outputCount);

	searchThreadData.Alloc(firstLayerOutputCount);

	srand((unsigned int)time(0)); // Randomize

	if (checkerBoard.bActive) {
		get_hashsize(&TTable.sizeMb);
		get_dbpath(checkerBoard.db_path, sizeof(checkerBoard.db_path));
		get_enable_wld(&checkerBoard.enable_wld);
		get_book_setting(&checkerBoard.useOpeningBook);
		get_dbmbytes(&checkerBoard.wld_cache_mb);
		get_max_dbpieces(&checkerBoard.max_dbpieces);
	}

	if (!TTable.SetSizeMB(TTable.sizeMb))
		TTable.SetSizeMB(64);

	if (!checkerBoard.bActive || !openingBook->Load("engines\\opening.gbk"))
		openingBook->Load("opening.gbk");

	// Load Endgame Databases, but not if running under CheckerBoard.
	// With CheckerBoard, check for loading or re-configuring egdbs in getmove().
	if (!checkerBoard.bActive && !dbInfo.loaded && checkerBoard.enable_wld)
	{
		if (status_str)
			sprintf(status_str, "Initializing endgame db...");

		InitializeEdsDatabases(dbInfo);

		if (!dbInfo.loaded) {
			InitializeGuiDatabases(dbInfo);
		}

		if (status_str)
			sprintf(status_str, "Initialized endgame db.");
	}

	board.StartPosition();
	transcript.Init(board);

	if (!checkerBoard.bActive)
	{
		// Create a thinking thread so we don't lock up the GUI
		static DWORD ThreadId;
		hAction = CreateEvent(NULL, FALSE, FALSE, NULL);
		hThread = CreateThread(NULL, 0, ThinkingThread, 0, 0, &ThreadId);	
		if (hThread == NULL) {
			GUI.ShowErrorPopup("Cannot Create Thread");
		}
	}
}

void Engine::MoveNow()
{
	if (bThinking) {
		bStopThinking = true;
		WaitForSingleObject(hEngineReady, 500);
	}
}

void Engine::StartThinking()
{
	WaitForSingleObject(hEngineReady, 1000);
	engine.bStopThinking = false;
	engine.bThinking = true;
	engine.searchLimits.newIterationMaxTime = engine.searchLimits.maxSeconds * .60;
	SetEvent(hAction);
}