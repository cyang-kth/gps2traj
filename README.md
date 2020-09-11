### gps2traj

`gps2traj` is a command line tool to convert GPS data (each row is a point containing id, x, y, timestamp fields) to trajectory format (each row is a LineString/trip). Both input and output are in CSV format.

The tool will partition GPS points according to id field and sort by timestamp.

```
# Input
id,x,y,timestamp
1,0,0,3
1,0,1,2
1,1,1,1

# Output
index;id;geom
1;1;LineString(1 1,0 1,0 0)
```

Test on a CSV file with 30 million rows takes about 84.797 seconds.

#### Usage of gps2traj

- `-i/--input`: input file
- `-o/--output`: output file
- `-d/--delim`: delimiter character (default `,`)
- `--id`: id column name (default `id`)
- `-x/--x`: x column name or index (default `x`)
- `-y/--y`: y column name or index (default `y`)
- `-t/--time`: timestamp column name or index (default `timestamp`)
- `--no_header`: if specified, gps file contains no header
- `--time_gap`: time gap to split too long trajectories (default 1e9)
- `--dist_gap`: distance gap to split too long trajectories (default 1e9)
- `--ofields`: output fields (ts,tend,timestamp) separated by , (default "")

#### Run example

```bash
cd example
gps2traj -i gps.csv -o traj.csv --time_gap 10
gps2traj -i gps_semicolon.csv -o traj.csv --time_gap 10 -d ';'
gps2traj -i gps_no_header.csv -o traj_timestamp.csv --time_gap 10 --no_header --id 0 -x 2 -y 3 -t 1 --ofields ts,tend,timestamp
```

### traj2gps

This tool transform a trajectory with linestring geometry into points.

```
# Input
id;geom
1;LineString(1 1,0 1,0 0)

# Output
id;point_idx;x;y
1;0;1;1
1;1;0;1
1;2;0;0
```

#### Usage of traj2gps

- `-i/--input`: input file
- `-o/--output`: output file
- `-d/--delim`: field delimiter character (default `;`)
- `--id`: id column name (default `id`)
- `-g/--geom`: geom column name or index (default `geom`)
- `--no_header`: if specified, traj file contains no header

#### Run example

```bash
cd example
traj2gps -i traj_input.csv -o gps_output.csv
```

#### Build and install

Run the command in bash shell at the project folder

```
make
make install
```

You may need root permission to run the second command `sudo make install`


#### Dependency

- Unix environment
- C++11

#### Contact

Can Yang, Email: cyang(at)kth.se
