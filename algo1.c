#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>

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

	int			is_walkable_frontier;
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

t_cell	*get_cell(t_map *map, int x, int y)
{
//    fprintf(stderr, "get_cell x, y : %d, %d. cells ptr : %p\n", x, y, map->cells);
	return (map->cells + (y * map->w + x));
}

// NOT actual distance. Foregoing sqrt since this only serves as a comparisson indicator.
float	dist_between(t_cell *c1, t_cell *c2)
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
		printf("SPAWN %d %d;", target->x, target->y);
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

int		is_target_cell(t_map *map, t_cell *cell)
{
	int	i;

	i = -1;
	while (++i < map->nb_target_cells)
		if (cell == map->target_cells[i])
			return (1);
	return (0);
}

t_cell	*__pop_frontier_cell_to_targets(t_map *map, int i)
{
	t_cell	*front;

	front = map->frontier_cells[0];
	memcpy(map->frontier_cells + i, map->frontier_cells + i + 1, sizeof(t_cell *) * (map->nb_frontier_cells - i));
	map->frontier_cells[--map->nb_frontier_cells] = NULL;
	map->target_cells[map->nb_target_cells++] = map->frontier_cells[i];
	return (front);
}

t_cell	*__put_spawn_cell_in_targets(t_map *map, t_cell *spawn)
{
	map->target_cells[map->nb_target_cells++] = spawn;
	return (spawn);
}

t_cell	*find_best_spawn(t_map *map)
{
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
	if (best_cell)
	{
		if (best_cell->left->owner == O_PLAYER)
			best_cell = __put_spawn_cell_in_targets(map, best_cell->left);
		else if (best_cell->right->owner == O_PLAYER)
			best_cell = __put_spawn_cell_in_targets(map, best_cell->right);
		else if (best_cell->up->owner == O_PLAYER)
			best_cell = __put_spawn_cell_in_targets(map, best_cell->up);
		else if (best_cell->down->owner == O_PLAYER)
			best_cell = __put_spawn_cell_in_targets(map, best_cell->down);
	}
	return (best_cell);
}

t_cell	*pop_best_move_for_bot(t_map *map, t_cell *bot)
{
	t_cell	**cells = map->frontier_cells;
	float	shortest_dist = 999999999.9f;
	float	curr_dist = 0;
	int		i = 0;
	int		best_idx = -1;
	t_cell	*best_cell;
	
	fprintf(stderr, "pop_best_move : entered : map %p, bot %p\n", map, bot);

	if (is_frontier_cell(map, bot->left, &i))
		return (__pop_frontier_cell_to_targets(map, i));
	else if (is_frontier_cell(map, bot->right, &i))
		return (__pop_frontier_cell_to_targets(map, i));
	else if (is_frontier_cell(map, bot->up, &i))
		return (__pop_frontier_cell_to_targets(map, i));
	else if (is_frontier_cell(map, bot->down, &i))
		return (__pop_frontier_cell_to_targets(map, i));
	fprintf(stderr, "pop_best_move : no \n", map, bot);
	best_cell = NULL;
	while (i < map->nb_frontier_cells)
	{
		curr_dist = dist_between(cells[i], bot);
		if (curr_dist < shortest_dist)
		{
			shortest_dist = curr_dist;
			best_cell = cells[i];
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
	
	if (!cell)
	{
//		fprintf(stderr, "is_walkable_frontier : NO CELL\n");
		return (0);
	}
	if (cell->scrap_amount && (cell->owner != O_PLAYER) && !cell->units)
	{
		cell->is_walkable_frontier = 1;
		i = -1;
		while (++i < map->nb_frontier_cells)
			if (map->frontier_cells[i] == cell)
				return (0);
		cell->travel_potential_score = 0;
        fprintf(stderr, "Calling traveling score calculator\n");
		traveling_score_calculator(cell, &cell->travel_potential_score);
        fprintf(stderr, "Calling traveling score calculator returned\n");
		return (1);
	}
	return (0);
}

void	score_cell_recycler_potential(t_map *map, t_cell *cell)
{
	int	amount;
	int	rate;

	if (!cell)
	{
		fprintf(stderr, "score_cell_recycler : NO CELL\n");
		return ;
	}

	if (!cell->can_build)
	{
		cell->rec_potential_amnt = 0;
		cell->rec_potential_rate = 0;
		return ;
	}

	amount = cell->scrap_amount;
	cell->rec_potential_rate++;
	if (cell->left && cell->left->scrap_amount > 0)	
	{
		cell->rec_potential_amnt += cell->left->scrap_amount;
		cell->rec_potential_rate++;
	}
	if (cell->right && cell->right->scrap_amount > 0)	
	{
		cell->rec_potential_amnt += cell->right->scrap_amount;
		cell->rec_potential_rate++;
	}
	if (cell->up && cell->up->scrap_amount > 0)	
	{
		cell->rec_potential_amnt += cell->up->scrap_amount;
		cell->rec_potential_rate++;
	}
	if (cell->down && cell->down->scrap_amount > 0)	
	{
		cell->rec_potential_amnt += cell->down->scrap_amount;
		cell->rec_potential_rate++;
	}
	if (cell->rec_potential_amnt > 35 && cell->rec_potential_rate > 2)
		map->build_cells[map->nb_build_cells++] = cell;
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
        fprintf(stderr, "opp cell at : (%d, %d)\n", cell->x, cell->y);
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
        fprintf(stderr, "ally cell at : (%d, %d)\n", cell->x, cell->y);
		map->nb_ally_patches += 1;
		if (cell->units > 0)
		{
			map->ally_cells[map->nb_ally_occupied++] = cell;
			map->nb_ally_units += cell->units;
		}

		// STACKS CURRENT FRONTIER CELLS
        fprintf(stderr, "starting walkable frontier tests \n");
        if (cell->recycler)
            map->nb_ally_recyclers += 1;
		if (is_walkable_frontier(map, cell->left))
			map->frontier_cells[map->nb_frontier_cells++] = cell->left;
		if (is_walkable_frontier(map, cell->right))
			map->frontier_cells[map->nb_frontier_cells++] = cell->right;
		if (is_walkable_frontier(map, cell->up))
			map->frontier_cells[map->nb_frontier_cells++] = cell->up;
		if (is_walkable_frontier(map, cell->down))
			map->frontier_cells[map->nb_frontier_cells++] = cell->down;
        fprintf(stderr, "all walkable frontier tests DONE\n");
		
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
	while (++boti < map->nb_ally_occupied)
	{
    	fprintf(stderr, "boti : %d\n", boti);
		ally = map->ally_cells[boti];
		while (ally->units--)
		{
    		fprintf(stderr, "ally->units : %d\n", ally->units);
			target = pop_best_move_for_bot(map, ally);
    		fprintf(stderr, "best target found : %p\n", target);
    		fprintf(stderr, "best target found : (x, y) : (%d, %d)\n", target->x, target->y);
			if (target)
				move_bot_to(ally, target);
		}
	}
    fprintf(stderr, "PHASE TWO STARTS : BUILD \n");
	/// PHASE TWO : BUILD
	while (map->my_matter >= 10)
	{
		if (map->nb_ally_recyclers < map->nb_ally_units)
		{
			if (map->nb_build_cells)
			{
				target = map->build_cells[--map->nb_build_cells];
				build_recycler_at(target);
			}
		}
		else
		{
			target = find_best_spawn(map);
			if (target)
				spawn_bot_at(target);
		}
	}
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
        delta_time = timer_us(&t0);
        fprintf(stderr, "delta_time : %zd\n", delta_time);
		game_plan(&map);
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
