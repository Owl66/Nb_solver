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


			"wikipedia_hard", 10, 9,

			"2        2\n"

			"      2   \n"

			" 2  7     \n"

			"          \n"

			"      3 3 \n"

			"  2    3  \n"

			"2  4      \n"

			"          \n"

			" 1    2 4 \n"
		},
    } };

	try {
		for (auto const& puzzle : puzzles) {
			auto const start = std::chrono::steady_clock::now();
			Grid g(puzzle.w, puzzle.h, puzzle.s);
			while(g.solve() == Grid::SitRep::KEEP_GOING) { }
			/*
			* The below function must run in parallel with solve function.
			LOG("Puzzle is solved...");
			*/


			auto const finish = std::chrono::steady_clock::now();

			ofstream f(puzzle.name + string(".html"));
			g.write(f, start, finish);

			cout << puzzle.name << std::endl;
			LOG(" Puzzle took " + format_time(start, finish));

			const int k = g.known();
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
