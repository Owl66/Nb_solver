#include <iostream>
#include <array>
#include <fstream>
#include "Log.hpp"
#include "Grid.hpp"

using namespace std;
using namespace nb_s;

int main()
{
	struct Puzzle {
		const char* name;
		int w;
		int h;
		const char* s;
	};

	const std::array<Puzzle, 1> puzzles = { {
		{


			 "nikoli_10", 36, 20,

			"           4            2           \n"

			"3 4          2   7         8      2 \n"

			"    7      5   1   8 5   1  2  4   2\n"

			"6    4       3          2 2         \n"

			"           6                   4    \n"

			"    2             1  2           2  \n"

			"        1       4     4    4  1     \n"

			" 1                  3            4 4\n"

			"     2     4  4            4        \n"

			"       5  3                   2 4   \n"

			" 5 1              1    3   8   2    \n"

			"     1   2                          \n"

			"2            2 5           4     2 1\n"

			"                             2      \n"

			"1  2   4  7   18   1            1   1\n"

			"                     2   8 4        \n"

			"    3           18     1          4  \n"

			"                 4                4 \n"

			"      3 1   4      4    2    4   4  \n"

			"6      1  3                 4       \n"


		},
    } };

	try {
		for (auto const& puzzle : puzzles) {
			auto const start = std::chrono::steady_clock::now();
			Grid g(puzzle.w, puzzle.h, puzzle.s);
			LOG("[INFO] we are working on it...");
			while(g.solve() == Grid::SitRep::KEEP_GOING) { }
			

			auto const finish = std::chrono::steady_clock::now();

			ofstream f(puzzle.name + string(".html"));
			g.write(f, start, finish);

			cout << puzzle.name << std::endl;
			LOG(" Puzzle took " + format_time(start, finish));

			const int k = g.knownElements();
			const int cells = puzzle.w * puzzle.h;
			cout << k << "/" << cells << " (" << k * 100.0 / cells << "%) solved" << endl;

		}

	}
	catch (exception const& e) {
		cerr << "exception caught " << e.what();
		return EXIT_FAILURE;

	}
	catch (...) {
		cerr << "unknown exception caught ";
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
