#!/usr/bin/perl

use strict;
use Getopt::Long;
use Env;
use File::Basename;
use File::copy;
use File::Spec;
use File::Path;
use Cwd;

my $eos_home = defined $ENV{EOS_HOME} ? $ENV{EOS_HOME} : getcwd;
my $eosd = $eos_home . "/programs/eosd/eosd";

my $nodes = defined $ENV{EOS_TEST_RING} ? $ENV{EOS_TEST_RING} : "1";
my $only_one = defined $ENV{EOS_TEST_ONE_PRODUCER} ? "1" : "";

my $prods = 21;
my $genesis = "$eos_home/genesis.json";
my $http_port_base = 8888;
my $p2p_port_base = 9876;
my $data_dir_base = './test-dir-node';
my $http_port_base = 8888;
my $hostname = "localhost";
my $first_pause = 45;
my $launch_pause = 5;
my $run_duration = 60;
my $topo = "ring";
my $override_gts = "now";

if (!GetOptions("nodes=i" => \$nodes,
                "first-pause=i" => \$first_pause,
                "launch-pause=i" => \$launch_pause,
                "duration=i" => \$run_duration,
                "one-producer" => \$only_one)) {
    print "usage: $ARGV[0] [--nodes=<n>] [--first-pause=<n>] [--launch-pause=<n>] [--duration=<n>] [--one-producer]\n";
    print "where:\n";
    print "--nodes=n (default = 1) sets the number of eosd instances to launch\n";
    print "--first-pause=n (default = 45) sets the seconds delay after starting the first instance\n";
    print "--launch-pause=n (default = 5) sets the seconds delay after starting subsequent nodes\n";
    print "--duration=n (default = 60) sets the seconds delay after starting the last node before shutting down the test\n";
    print "--one-producer (default = no) if set concentrates all producers into the first node\n";
    print "\nproducer count currently fixed at $prods\n";
    exit
}

my $per_node = int ($prods / ($only_one ? 1 : $nodes));
my $extra = $prods - ($per_node * $nodes);
my @pid;
my @data_dir;
my $prod_ndx = ord('a');
my @p2p_port;
my @http_port;
my @peers;
my $rhost = $hostname; # from a list for multihost tests
for (my $i = 0; $i < $nodes; $i++) {
    $p2p_port[$i] = $p2p_port_base + $i;
    $http_port[$i] = $http_port_base + $i;
    $data_dir[$i] = "$data_dir_base-$i";
}


sub write_config {
    my $i = shift;

    print "purging directory $data_dir[$i]\n";
    rmtree ($data_dir[$i]);
    mkdir ($data_dir[$i]);

    open (my $cfg, '>', "$data_dir[$i]/config.ini") ;
    print $cfg "genesis-json = \"$genesis\"\n";
    print $cfg "block-log-dir = \"blocks\"\n";
    print $cfg "readonly = 0\n";
    print $cfg "shared-file-dir = \"blockchain\"\n";
    print $cfg "shared-file-size = 64\n";
    print $cfg "http-server-endpoint = $hostname:$http_port[$i]\n";
    print $cfg "listen-endpoint = 0.0.0.0:$p2p_port[$i]\n";
    print $cfg "public-endpoint = $hostname:$p2p_port[$i]\n";
    foreach my $peer (@peers) {
        print $cfg "remote-endpoint = $peer\n";
    }

    print $cfg "enable-stale-production = true\n";
    print $cfg "required-participation = true\n";
    print $cfg "private-key = [\"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV\",\"5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3\"]\n";

    if ($i == 0 || $only_one != "1") {
        print $cfg "plugin = eos::producer_plugin\n";
        $per_node += $extra if ($i == $nodes - 1);
        for (my $p = 0; $p < $per_node; $p++) {
            my $pname = "init" . chr($prod_ndx++);
            print $cfg "producer-name = $pname\n";
        }
    }
    close $cfg;
}


sub make_ring_topology () {
    for (my $i = 0; $i < $nodes; $i++) {
        my $rport = ($i == $nodes - 1) ? $p2p_port_base : $p2p_port[$i] + 1;
        $peers[0] = "$rhost:$rport";
        if ($nodes > 2) {
            $rport = $p2p_port[$i] - 1;
            $rport += $nodes if ($i == 0);
            $peers[1] = "$rhost:$rport";
        }
        write_config ($i);
    }
    return 1;
}

sub make_grid_topology () {
    print "Sorry, the grid topology is not yet implemented\n";
    return 0;
}

sub make_star_topology () {
    print "Sorry, the star topology is not yet implemented\n";
    return 0;
 }

sub launch_nodes () {
    my $GTS = $override_gts;
    if ($override_gts =~ "now" ) {
        chomp ($GTS = `date -u "+%Y-%m-%dT%H:%M:%S"`);
        my @s = split (':',$GTS);
        $s[2] = substr ((100 + (int ($s[2]/3) * 3)),1);
        $GTS = join (':', @s);
        print "using genesis time stamp $GTS\n";
    }
    my $gtsarg;
    $gtsarg = "--genesis-timestamp=$GTS" if ($override_gts);

    for (my $i = 0; $i < $nodes;  $i++) {
        my @cmdline = ($eosd,
                       $gtsarg,
                       "--data-dir=$data_dir[$i]");

        $pid[$i] = fork;
        if ($pid[$i] > 0) {
            my $pause = $i == 0 ? $first_pause : $launch_pause;
            print "parent process looping, child pid = $pid[$i]";
            if ($i < $nodes - 1) {
                print ", pausing $pause seconds\n";
                sleep ($pause);
            }
            else {
                print "\n";
            }

        }
        elsif (defined ($pid[$i])) {
            print "child execing now, pid = $$\n";
            open OUTPUT, '>', "$data_dir[$i]/stdout.txt" or die $!;
            open ERROR, '>', "$data_dir[$i]/stderr.txt" or die $!;
            STDOUT->fdopen ( \*OUTPUT, 'w') or die $!;
            STDERR->fdopen ( \*ERROR, 'w') or die $!;

            exec @cmdline;
            print "child terminating now\n";
            exit;
        }
        else {
            print "fork failed\n";
            exit;
        }
    }
}

sub kill_nodes () {
    print "all nodes launched, network running for $run_duration seconds\n";
    sleep ($run_duration);

    foreach my $pp (@pid) {
        print "killing $pp\n";
        kill 2, $pp;
    }
}


###################################################
# main

if ($nodes == 1) {
    write_config (0);
}
else {
    if    ( $topo =~ "ring" ) { make_ring_topology () or die; }
    elsif ( $topo =~ "grid" ) { make_grid_topology () or die; }
    elsif ( $topo =~ "star" ) { make_star_topology () or die; }
    else  { print "$topo is not a known topology" and die; }
}

launch_nodes ();

kill_nodes () if ($run_duration > 0);
