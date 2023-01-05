#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>
#include <limits.h>

/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/

enum
{
    O_PLAYER = 1,
    O_OPPONENT = 0,
	O_NEUTRAL = -1
};

//typedef s_cell_data;

typedef struct timeval  t_tv;

typedef struct  s_cell_data
{
	int			x;
	int			y;

	struct s_cell_data	*up;
	struct s_cell_data	*down;
	struct s_cell_data	*left;
	struct s_cell_data	*right;

    int			scrap_amount;
    int			owner;
    int			units;
    int			recycler;
    int			can_build;
    int			can_spawn;
    int			in_rec_range;

	int			is_walkable;
	int			is_walkable_frontier;
//	int			is_target;
	int			rec_potential_amnt;
	int			rec_potential_rate;
	int			travel_potential_score;
}   t_cell;

typedef struct  s_map_data
{
	int		w;
	int		h;
    int     my_matter;
    int     opp_matter;
    t_cell  *cells;
	int		nb_enemy_units;
	int		nb_enemy_occupied;
	int		nb_enemy_patches;
	int		nb_enemy_recyclers;
	int		nb_ally_units;
	int		nb_ally_occupied;
	int		nb_ally_patches;
	int		nb_ally_recyclers;
	int		nb_frontier_cells;
	int		nb_target_cells;
	int		nb_build_cells;
	t_cell	**enemy_cells;
	t_cell	**ally_cells;
	t_cell	**frontier_cells;
	t_cell	**target_cells;
	t_cell	**build_cells;
}   t_map;


void	print_ally_cells(t_map *map)
{
	int	i;

	fprintf(stderr, "Ally cells (%d):\n", map->nb_ally_occupied);
	i = -1;
	while (++i < map->nb_ally_occupied)
	{
		fprintf(stderr, "\t - (%d, %d)\n", map->ally_cells[i]->x, map->ally_cells[i]->y);
	}
}

void	print_build_cells(t_map *map)
{
	int	i;

	fprintf(stderr, "Build cells (%d):\n", map->nb_build_cells);
	i = -1;
	while (++i < map->nb_build_cells)
	{
		fprintf(stderr, "\t - (%d, %d)\n", map->build_cells[i]->x, map->build_cells[i]->y);
	}
}

void	print_frontier_cells(t_map *map)
{
	int	i;

	fprintf(stderr, "Frontier cells (%d):\n", map->nb_frontier_cells);
	i = -1;
	while (++i < map->nb_frontier_cells)
	{
		fprintf(stderr, "\t - (%d, %d)\n", map->frontier_cells[i]->x, map->frontier_cells[i]->y);
	}
}

void	print_enemy_cells(t_map *map)
{
	int	i;

	fprintf(stderr, "Enemy cells (%d):\n", map->nb_enemy_occupied);
	i = -1;
	while (++i < map->nb_enemy_occupied)
	{
		fprintf(stderr, "\t - (%d, %d)\n", map->enemy_cells[i]->x, map->enemy_cells[i]->y);
	}
}

t_cell	*get_cell(t_map *map, int x, int y)
{
//    fprintf(stderr, "get_cell x, y : %d, %d. cells ptr : %p\n", x, y, map->cells);
	return (map->cells + (y * map->w + x));
}

// Gives better score if cell is near the middle of the map on the x axis.
int	bell_curve_rec_score_bonus(t_map *map, t_cell *cell)
{
	if (!cell->recycler)
		return (0);
	
	int		xmu = (cell->x - (map->w / 2));
	return ((int)(10.0f * expf(-2 * xmu * xmu)));
}

// NOT actual distance. Foregoing sqrt since this only serves as a comparisson indicator.
int	dist_between(t_cell *c1, t_cell *c2)
{
	int	dx, dy;

	dx = c2->x - c1->x;
	dy = c2->y - c1->y;
	return (dx * dx + dy * dy);
}

void	move_bot_to(t_cell *bot, t_cell *target)
{
	if (target->scrap_amount)
		printf("MOVE 1 %d %d %d %d;", bot->x, bot->y, target->x, target->y);
}

void	spawn_bot_at(t_cell *target)
{
	if (target->can_spawn)
		printf("SPAWN 1 %d %d;", target->x, target->y);
}

void	build_recycler_at(t_cell *target)
{
	if (target->can_build)
		printf("BUILD %d %d;", target->x, target->y);
}

int		is_frontier_cell(t_map *map, t_cell *cell, int *idx)
{
	int	i;

	*idx = -1;
	i = -1;
	while (++i < map->nb_frontier_cells)
	{
		if (cell == map->frontier_cells[i])
		{
			*idx = i;
			return (1);
		}
	}
	return (0);
}

/*
int		is_target_cell(t_map *map, t_cell *cell)
{
	int	i;

	i = -1;
	while (++i < map->nb_target_cells)
		if (cell == map->target_cells[i])
			return (1);
	return (0);
}
*/
t_cell	*__pop_frontier_cell_to_targets(t_map *map, int i)
{
	t_cell	*front;

	front = map->frontier_cells[i];
	if (!front)// || is_target_cell(map, front))
		return (NULL);
	memcpy(map->frontier_cells + i, map->frontier_cells + i + 1, sizeof(t_cell *) * (map->nb_frontier_cells - i));
	map->frontier_cells[--map->nb_frontier_cells] = NULL;
//	map->target_cells[map->nb_target_cells++] = map->frontier_cells[i];
	return (front);
}

t_cell	*__put_spawn_cell_in_targets(t_map *map, t_cell *spawn)
{
//	if (map->nb_target_cells)
	map->target_cells[map->nb_target_cells++] = spawn;
	return (spawn);
}

t_cell	*find_best_spawn(t_map *map)
{
	static int	ally_spawn_idx = 0;
	t_cell	**cells = map->frontier_cells;
	int		best_score = -1;
	int		i = 0;
	int		best_idx = -1;
	t_cell	*best_cell;

	best_cell = NULL;
	while (i < map->nb_frontier_cells)
	{
		if (cells[i]->travel_potential_score > best_score)
		{
			best_score = cells[i]->travel_potential_score;
			best_cell = cells[i];
			best_idx = i;
		}
		i++;
	}
	if (best_cell && !best_cell->recycler)
	{
		fprintf(stderr, "Best spawn cell found\n");
		if (best_cell->left && best_cell->left->owner == O_PLAYER)
			best_cell = best_cell->left;
//			best_cell = __put_spawn_cell_in_targets(map, best_cell->left);
		else if (best_cell->right && best_cell->right->owner == O_PLAYER)
			best_cell = best_cell->right;
//			best_cell = __put_spawn_cell_in_targets(map, best_cell->right);
		else if (best_cell->up && best_cell->up->owner == O_PLAYER)
			best_cell = best_cell->up;
//			best_cell = __put_spawn_cell_in_targets(map, best_cell->up);
		else if (best_cell->down && best_cell->down->owner == O_PLAYER)
			best_cell = best_cell->down;
//			best_cell = __put_spawn_cell_in_targets(map, best_cell->down);
		fprintf(stderr, "Best spawn : GOT'EM !\n");
	}
	else
	{
		fprintf(stderr, "Best spawn cell NOT found\n");
		if (map->nb_ally_occupied)
			best_cell = map->ally_cells[ally_spawn_idx % map->nb_ally_occupied];
		ally_spawn_idx++;
	}
	return (best_cell);
}

t_cell	*pop_best_move_for_bot(t_map *map, t_cell *bot)
{
	t_cell	**cells = map->frontier_cells;
	int		shortest_dist = INT_MAX;
	int		curr_dist = 0;
	int		i = 0;
	int		best_idx = -1;
	t_cell	*best_cell;
	
	fprintf(stderr, "pop_best_move : entered : map %p, bot %p\n", map, bot);
	fprintf(stderr, "pop_best_move : nb_frontier_cells : %d\n", map->nb_frontier_cells);

	if (is_frontier_cell(map, bot->left, &i))
		return (__pop_frontier_cell_to_targets(map, i));
	else if (is_frontier_cell(map, bot->right, &i))
		return (__pop_frontier_cell_to_targets(map, i));
	else if (is_frontier_cell(map, bot->up, &i))
		return (__pop_frontier_cell_to_targets(map, i));
	else if (is_frontier_cell(map, bot->down, &i))
		return (__pop_frontier_cell_to_targets(map, i));
//	fprintf(stderr, "pop_best_move : no \n", map, bot);
	best_cell = NULL;
	i = 0;
	while (i < map->nb_frontier_cells)
	{
		curr_dist = dist_between(map->frontier_cells[i], bot);
		if (curr_dist < shortest_dist && bot != map->frontier_cells[i])
		{
			shortest_dist = curr_dist;
			best_cell = map->frontier_cells[i];
			best_idx = i;
		}
		i++;
	}

	fprintf(stderr, "pop_best_move : best_cell : %p\n", best_cell);
	if (best_cell)
		__pop_frontier_cell_to_targets(map, i);
	else if (map->enemy_cells[0])
	{
		best_cell = map->enemy_cells[0];
		memcpy(map->enemy_cells, map->enemy_cells + 1, sizeof(t_cell *) * (map->nb_enemy_occupied - 1));
		map->enemy_cells[--map->nb_enemy_occupied] = NULL;
	}
	return (best_cell);
}

void	traveling_score_calculator(t_cell *cell, int *score)
{
	t_cell	*c;
	
	if (!cell)
	{
		fprintf(stderr, "traveling_score_calculator : NO CELL\n");
		return ;
	}
//    fprintf(stderr, "traveling score calculator entered. Score ptr : %p, score value : %d\n", score, *score);
	if (cell->is_walkable_frontier || (cell->scrap_amount && (cell->owner != O_PLAYER) && !cell->units))
	{
//		fprintf(stderr, "traveling score calculator : is valid frontier\n");
		(*score)++;
//		fprintf(stderr, "traveling score calculator cell->left : %p\n", cell->left);
		c = cell->left;
		*score += (!!c && (c->is_walkable_frontier || (c->scrap_amount && (c->owner != O_PLAYER) && !c->units)));
//		fprintf(stderr, "traveling score calculator cell->right : %p\n", cell->right);
		c = cell->right;
		*score += (!!c && (c->is_walkable_frontier || (c->scrap_amount && (c->owner != O_PLAYER) && !c->units)));
//		fprintf(stderr, "traveling score calculator cell->up : %p\n", cell->up);
		c = cell->up;
		*score += (!!c && (c->is_walkable_frontier || (c->scrap_amount && (c->owner != O_PLAYER) && !c->units)));
//		fprintf(stderr, "traveling score calculator cell->down : %p\n", cell->down);
		c = cell->down;
		*score += (!!c && (c->is_walkable_frontier || (c->scrap_amount && (c->owner != O_PLAYER) && !c->units)));
//		fprintf(stderr, "traveling score calculator : score ptr, cell ptr : %p, %p\n", score, cell);
		if (score == &cell->travel_potential_score)
		{
//    	    fprintf(stderr, "traveling score going recursive\n");
			traveling_score_calculator(cell->left, score);
			traveling_score_calculator(cell->right, score);
			traveling_score_calculator(cell->up, score);
			traveling_score_calculator(cell->down, score);
		}
	}
}

int	is_walkable_frontier(t_map *map, t_cell *cell)
{
	int		i;
	
	if (!cell || !cell->is_walkable)
	{
//		fprintf(stderr, "is_walkable_frontier : NO CELL\n");
		return (0);
	}
	if (cell->scrap_amount && (cell->owner != O_PLAYER))// && !cell->units)
	{
		if (is_frontier_cell(map, cell, &i))
		{
			fprintf(stderr, "is walkable : cell (%d, %d) discarded for being frontier cell already\n", cell->x, cell->y);
			return (0);
		}
		cell->is_walkable_frontier = 1;
		cell->travel_potential_score = 0;
//        fprintf(stderr, "Calling traveling score calculator\n");
		traveling_score_calculator(cell, &cell->travel_potential_score);
//        fprintf(stderr, "Calling traveling score calculator returned\n");
		return (1);
	}
	return (0);
}

void	score_cell_recycler_potential(t_map *map, t_cell *cell)
{
	int	amount;
	int	rate;

//	fprintf(stderr, "score rec pot : cell (%d, %d) evaluation\n", cell->x, cell->y);
	if (!cell)
	{
		fprintf(stderr, "score_cell_recycler : NO CELL\n");
		return ;
	}
	cell->rec_potential_amnt = 0;
	cell->rec_potential_rate = 0;
	if (!cell->can_build || cell->in_rec_range)
		return ;

//	fprintf(stderr, "score rec pot : cell (%d, %d) can build.\n", cell->x, cell->y);
	amount = cell->scrap_amount + bell_curve_rec_score_bonus(map, cell);
	cell->rec_potential_rate++;
	if (cell->left && cell->left->scrap_amount > 0)	
	{
		cell->rec_potential_amnt += cell->left->scrap_amount;
		cell->rec_potential_rate += 1 + (cell->left->owner == O_OPPONENT);
	}
	if (cell->right && cell->right->scrap_amount > 0)	
	{
		cell->rec_potential_amnt += cell->right->scrap_amount;
		cell->rec_potential_rate += 1 + (cell->right->owner == O_OPPONENT);
	}
	if (cell->up && cell->up->scrap_amount > 0)	
	{
		cell->rec_potential_amnt += cell->up->scrap_amount;
		cell->rec_potential_rate += 1 + (cell->up->owner == O_OPPONENT);
	}
	if (cell->down && cell->down->scrap_amount > 0)	
	{
		cell->rec_potential_amnt += cell->down->scrap_amount;
		cell->rec_potential_rate += 1 + (cell->down->owner == O_OPPONENT);
	}
//	fprintf(stderr, "Recycler potentil cell (%d, %d) : amnt %d, rate %d\n", cell->x, cell->y, cell->rec_potential_amnt, cell->rec_potential_rate);	
	if (cell->rec_potential_amnt > 25 && cell->rec_potential_rate > 2)
	{
		fprintf(stderr, "Cell (%d, %d) suited to build. Added.\n", cell->x, cell->y);
		if (map->nb_build_cells && map->build_cells[0]->rec_potential_amnt < cell->rec_potential_amnt)
		{
			memmove(map->build_cells + 1, map->build_cells, sizeof(t_cell *) * map->nb_build_cells);
			map->build_cells[0] = cell;
		}
		else
			map->build_cells[map->nb_build_cells++] = cell;
	}
}

void	print_map_info(t_map *map)
{
	fprintf(stderr, "@@______________(MAP %p)______________@@\n", map);
	fprintf(stderr, "@@	- Shape (width x height): (%d x %d)\n", map->w, map->h);
	fprintf(stderr, "@@	      	        |	ALLY     |	ENEMY	|\n");
	fprintf(stderr, "@@	- matter :	|	 %-8d|	%-8d|\n", map->my_matter, map->opp_matter);
	fprintf(stderr, "@@	- units :	|	 %-8d|	%-8d|\n", map->nb_ally_units, map->nb_enemy_units);
	fprintf(stderr, "@@	- occupied :    |	 %-8d|	%-8d|\n", map->nb_ally_occupied, map->nb_enemy_occupied);
	fprintf(stderr, "@@	- patches:      |	 %-8d|	%-8d|\n", map->nb_ally_patches, map->nb_enemy_patches);
	fprintf(stderr, "@@	- recyclers:      |	 %-8d|	%-8d|\n", map->nb_ally_recyclers, map->nb_enemy_recyclers);
	fprintf(stderr, "@@______________________________________@@\n\n");
}

int init_map(t_map *map)
{
	t_cell	*cell;
	int	nb_cells;
	int	i, j;

	if (!map)
		return (-1);

	scanf("%d%d", &map->w, &map->h);
	nb_cells = map->w * map->h;
	map->cells = calloc(nb_cells, sizeof(t_cell));
	map->enemy_cells = (t_cell **)malloc(nb_cells * sizeof(t_cell *));
	map->ally_cells = (t_cell **)malloc(nb_cells * sizeof(t_cell *));
	map->frontier_cells = (t_cell **)malloc(nb_cells * sizeof(t_cell *));
	map->target_cells = (t_cell **)malloc(nb_cells * sizeof(t_cell *));
	map->build_cells = (t_cell **)malloc(nb_cells * sizeof(t_cell *));
	map->enemy_cells[0] = NULL;
	map->ally_cells[0] = NULL;
	map->frontier_cells[0] = NULL;
    if (!map->cells || !map->enemy_cells || !map->ally_cells)
    {
	    fprintf(stderr, "MALLOC ERROR !!\n");
        return (-1);
    }
//    fprintf(stderr, "Malloced cells ptr : %p\n", map->cells);


	i = 0;
	while (i < map->h)
	{
		j = 0;
		while (j < map->w)
		{
			cell = get_cell(map, j, i);
            cell->x = j;
            cell->y = i;
			if (i - 1 >= 0)
				cell->up = get_cell(map, j, i - 1);
			if (i + 1 < map->h)
				cell->down = get_cell(map, j, i + 1);
			if (j - 1 >= 0)
				cell->left = get_cell(map, j - 1, i);
			if (j + 1 < map->w)
				cell->right = get_cell(map, j + 1, i);
			j++;
		}
		i++;
	}
    return (0);
}

void	update_map_info_with_cell(t_map *map, t_cell *cell)
{
//    fprintf(stderr, "update_map_info : map %p, cell %p, cell x, y: (%d, %d)\n", map, cell, cell->x, cell->y);
	if (cell->owner == O_OPPONENT)
	{
//        fprintf(stderr, "opp cell at : (%d, %d)\n", cell->x, cell->y);
		map->nb_enemy_patches += 1;
		if (cell->units > 0)
		{
			map->enemy_cells[map->nb_enemy_occupied++] = cell;
			map->nb_enemy_units += cell->units;
		}
        if (cell->recycler)
            map->nb_enemy_recyclers++;
	}
	else if (cell->owner == O_PLAYER)
	{
//		fprintf(stderr, "ally cell at : (%d, %d)\n", cell->x, cell->y);
		map->nb_ally_patches += 1;
		if (cell->units > 0)
		{
			map->ally_cells[map->nb_ally_occupied++] = cell;
			map->nb_ally_units += cell->units;
		}

		// STACKS CURRENT FRONTIER CELLS
//        fprintf(stderr, "starting walkable frontier tests \n");
        if (cell->recycler)
            map->nb_ally_recyclers += 1;
	}
	cell->is_walkable = (cell->scrap_amount && !cell->recycler);
}

void	find_frontier_cells(t_map *map)
{
	t_cell	*cell;
	int	i;
	int	j;

	i = -1;
	while (++i < map->h)
	{
		j = -1;
		while (++j < map->w)
		{
			cell = get_cell(map, j, i);
			if (!cell)
			{
				fprintf(stderr, "cell (%d, %d) can't be found in map\n", j, i)
				continue ;
			}
			if (cell->owner != O_PLAYER)
				continue ;
			if (fprintf(stderr, "checking if cell->left (%d, %d) from cell (%d, %d) is walkable frontier\n", cell->left->x, cell->left->y, cell->x, cell->y)
				&& is_walkable_frontier(map, cell->left))
			{
				fprintf(stderr, "It is !\n");
				map->frontier_cells[map->nb_frontier_cells++] = cell->left;
			}
			if (fprintf(stderr, "checking if cell->right (%d, %d) from cell (%d, %d) is walkable frontier\n", cell->right->x, cell->right->y, cell->x, cell->y)
				&& is_walkable_frontier(map, cell->right))
			{
				fprintf(stderr, "It is !\n");
				map->frontier_cells[map->nb_frontier_cells++] = cell->right;
			}
			if (fprintf(stderr, "checking if cell->up (%d, %d) from cell (%d, %d) is walkable frontier\n", cell->up->x, cell->up->y, cell->x, cell->y)
				&& is_walkable_frontier(map, cell->up))
			{
				fprintf(stderr, "It is !\n");
				map->frontier_cells[map->nb_frontier_cells++] = cell->up;
			}
			if (fprintf(stderr, "checking if cell->down (%d, %d) from cell (%d, %d) is walkable frontier\n", cell->down->x, cell->down->y, cell->x, cell->y)
				&& is_walkable_frontier(map, cell->down))
			{
				fprintf(stderr, "It is !\n");
				map->frontier_cells[map->nb_frontier_cells++] = cell->down;
			}
		}
	}
}

void    reset_map_info(t_map *map)
{
    fprintf(stderr, "map info reset \n");
    map->nb_ally_occupied = 0;
    map->nb_ally_units = 0;
    map->nb_ally_patches = 0;
    map->nb_ally_recyclers = 0;
    map->nb_enemy_occupied = 0;
    map->nb_enemy_units = 0;
    map->nb_enemy_patches = 0;
    map->nb_enemy_recyclers = 0;
	map->nb_frontier_cells = 0;
	map->nb_target_cells = 0;
	map->nb_build_cells = 0;
}

ssize_t timer_us(t_tv *t0)
{
    t_tv    curr_t;
    ssize_t delta_t;

    if (!t0)
        return (0);
    gettimeofday(&curr_t, NULL);
    delta_t = (ssize_t)(curr_t.tv_sec - t0->tv_sec) * 1000000;
    delta_t += (ssize_t)(curr_t.tv_usec - t0->tv_usec);
    return (delta_t);
 }

void	game_plan(t_map *map)
{
	int		boti;
	t_cell	*ally;
	t_cell	*target;

    fprintf(stderr, "PHASE ONE STARTS : MOVE \n");
	/// PHASE ONE : MOVE
	boti = -1;
	int	no_more_targets = 0;
	while (++boti < map->nb_ally_occupied)
	{
		ally = map->ally_cells[boti];
    	fprintf(stderr, "boti : %d, cell (%d, %d), nb units %d\n", boti, ally->x, ally->y, ally->units);
		while (ally->units--)
		{
    		fprintf(stderr, "ally->units : %d\n", ally->units);
			target = pop_best_move_for_bot(map, ally);
    		fprintf(stderr, "best target found : %p\n", target);
			if (!target)
				break ;
    		fprintf(stderr, "best target found : (x, y) : (%d, %d)\n", target->x, target->y);
			move_bot_to(ally, target);
		}
	}
    fprintf(stderr, "PHASE TWO STARTS : BUILD \n");
	/// PHASE TWO : BUILD
	while (map->my_matter >= 10)
	{
    	fprintf(stderr, "Build loop matter amount : %d, nb build cells : %d\n", map->my_matter, map->nb_build_cells);
		if (map->nb_build_cells && map->nb_ally_recyclers < (map->nb_ally_units / 2))
		{
			target = map->build_cells[--map->nb_build_cells];
			build_recycler_at(target);
    		fprintf(stderr, "building recycler at (%d, %d)\n", target->x, target->y);
			map->my_matter -= 10;
		}
		else
		{
    		fprintf(stderr, "Else spawn bot \n");
			target = find_best_spawn(map);	// POTENTIAL INIFINIT LOOP WHEN ALMOST ALL CONQUERED
    		fprintf(stderr, "spawn target : %p\n", target);
			if (target)
			{
    			fprintf(stderr, "Spawning bot at (%d, %d)\n", target->x, target->y);
				spawn_bot_at(target);
				map->my_matter -= 10;
			}
		}
	}
    fprintf(stderr, "PHASE THREE STARTS : PROFIT$ \n");

	/// PHASE THREE : PROFIT$ !?
}

int main()
{
    static t_map	map;
	t_cell	*cell;
    ssize_t delta_time;

    if (init_map(&map) < 0)
        return (1);
    // game loop
    while (1) {
        reset_map_info(&map);
		
    	fprintf(stderr, "update loop started\n");
        t_tv    t0;
        gettimeofday(&t0, NULL);
        scanf("%d%d", &map.my_matter, &map.opp_matter);
        for (int i = 0; i < map.h; i++) {
        	fprintf(stderr, " line %d \n", i);

            for (int j = 0; j < map.w; j++) {
                // 1 = me, 0 = foe, -1 = neutral
//           	fprintf(stderr, "checking cell A\n");
				cell = get_cell(&map, j, i); 
//            	fprintf(stderr, "checking cell B %p\n", cell);
				scanf("%d%d%d%d%d%d%d", &cell->scrap_amount, &cell->owner,
					&cell->units, &cell->recycler, &cell->can_build, &cell->can_spawn,
					&cell->in_rec_range);

				update_map_info_with_cell(&map, cell);
				score_cell_recycler_potential(&map, cell);
            }
        }
		find_frontier_cells(&map);
        delta_time = timer_us(&t0);
        fprintf(stderr, "delta_time : %zd\n", delta_time);

		print_frontier_cells(&map);
		print_ally_cells(&map);
		print_enemy_cells(&map);
		print_build_cells(&map);
		game_plan(&map);
    	fprintf(stderr, "GAME PLAN OVER AND DONE !\n");
//		map.ally_cells[map.nb_ally_occupied] = NULL;
//		map.enemy_cells[map.nb_enemy_occupied] = NULL;
//		map.frontier_cells[map.nb_frontier_cells] = NULL;
//		map.target_cells[map.nb_target_cells] = NULL;
//		map.build_cells[map.nb_build_cells] = NULL;
		print_map_info(&map);
        // Write an action using printf(). DON'T FORGET THE TRAILING \n
        // To debug: fprintf(stderr, "Debug messages...\n");
        printf("WAIT\n");
    }

    return 0;
}
