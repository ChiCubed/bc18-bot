/*
 * A file for computation of
 * the nearest point in a set, by euclidean distance,
 * to each cell in the map.
 */

#include <vector>
#include <utility>
#include <cmath>
using namespace std;

namespace Voronoi
{
	int disToClosestEnemy[60][60];
	pair<int, int> closestEnemy[60][60];

	struct ConvexHullTrick
	{
		// note:
		// these only really need to be bounded by 60...
		// but whatever
		int M[1005], C[1005], ID[1005], ptr, len;

		// insert an element into the convex hull trick structure
		void insert(int m, int c, int id)
		{
			while (len > 1 && (C[len-2]-C[len-1]) * (m-M[len-1]) >= (C[len-1]-c) * (M[len-1]-M[len-2]))
			{
				--len;
			}
			M[len] = m; C[len] = c; ID[len] = id; ++len;
		}

		// return the value of the minimum line at the query point
		int query(int x)
		{
			ptr = min(ptr, len - 1);
			while (ptr < len - 1 && M[ptr+1] * x + C[ptr+1] <= M[ptr] * x + C[ptr])
			{
				++ptr;
			}
			return M[ptr] * x + C[ptr];
		}

		// return the ID of the minimum line at the given query point
		int get_curr_id()
		{
			return ID[ptr];
		}

		void clear()
		{
			len = ptr = 0;
		}
	} CHT;

	void findClosest(vector<pair<int, int>> &enemies, int r, int c)
	{
		int enemyAtLoc[60][60];
		pair<int, int> closestInCol[60][60];

		// (i, j) = (x, y) = (col, row)

		for (int i = 0; i < c; ++i) fill(enemyAtLoc[i], enemyAtLoc[i] + r, -1);

		for (int i = 0; i < enemies.size(); ++i)
		{
			enemyAtLoc[enemies[i].first][enemies[i].second] = i;
		}

		for (int i = 0; i < c; ++i)
		{
			for (int j = 0; j < r; ++j) closestInCol[i][j] = {-1, -1};

			int p = -1, n;
			for (n = 0; n < r && enemyAtLoc[i][n] == -1; ++n);

			if (n == r)
			{
				// no units here... rip
				continue;
			}

			for (int j = 0; j < r; ++j)
			{
				if (j == n)
				{
					p = n;
					for (++n; n < r && enemyAtLoc[i][n] == -1; ++n);
				}

				if (p == -1 || (n < r && abs(n - j) < abs(p - j)))
				{
					closestInCol[i][j] = {abs(n - j), enemyAtLoc[i][n]};
				}
				else
				{
					closestInCol[i][j] = {abs(p - j), enemyAtLoc[i][p]};
				}
			}
		}

		// j is row
		for (int j = 0; j < r; ++j)
		{
			CHT.clear();

			// i is col
			for (int i = 0; i < c; ++i)
			{
				if (closestInCol[i][j].first != -1)
				{
					CHT.insert(
						-2 * i,
						closestInCol[i][j].first * closestInCol[i][j].first + i * i,
						closestInCol[i][j].second
					);
				}
			}

			for (int i = 0; i < c; ++i)
			{
				disToClosestEnemy[i][j] = CHT.query(i) + i*i;
				closestEnemy[i][j] = enemies[CHT.get_curr_id()];
			}
		}
	}

	bool checker(vector<pair<int, int> > &enemies, int r, int c)
	{
		for (int i = 0; i < c; i++)
		{
			for (int j = 0; j < r; j++)
			{
				int dis = disToClosestEnemy[i][j];
				bool seen = false;
				for (auto a : enemies)
				{
					int d = pow(i-a.first, 2) + pow(j-a.second, 2);
					if (d < dis) return false;
					if (d == dis) seen = true;
				} 
				if (!seen) return false;
			}
		}
		return true;
	}
};