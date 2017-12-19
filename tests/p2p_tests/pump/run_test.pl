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
my $eosc = $eos_home . "/programs/eosc/eosc";

my $nodes = defined $ENV{EOS_TEST_RING} ? $ENV{EOS_TEST_RING} : "1";
my $pnodes = defined $ENV{EOS_TEST_PRODUCERS} ? $ENV{EOS_TEST_PRODUCERS} : "1";

my $prods = 21;
my $genesis = "$eos_home/genesis.json";
my $http_port_base = 8888;
my $p2p_port_base = 9876;
my $data_dir_base = "tdn";
my $hostname = "127.0.0.1";
my $first_pause = 0;
my $launch_pause = 0;
my $run_duration = 60;
my $topo = "ring";
my $override_gts = "";
my $no_delay=0;
my $test=1;

if (!GetOptions("nodes=i" => \$nodes,
                "first-pause=i" => \$first_pause,
                "launch-pause=i" => \$launch_pause,
                "duration=i" => \$run_duration,
                "topo=s" => \$topo,
                "test=i" => \$test,
                "time-stamp=s" => \$override_gts,
                "pnodes=i" => \$pnodes)) {
    print "usage: $ARGV[0] [--nodes=<n>] [--pnodes=<n>] [--topo=<ring|star>] [--first-pause=<n>] [--launch-pause=<n>] [--duration=<n>] [--time-stamp=<time> \n";
    print "where:\n";
    print "--nodes=n (default = 1) sets the number of eosd instances to launch\n";
    print "--pnodes=n (default = 1) sets the number nodes that will also be producers\n";
    print "--topo=s (default = ring) sets the network topology to either a ring shape or a star shape\n";
    print "--first-pause=n (default = 0) sets the seconds delay after starting the first instance\n";
    print "--launch-pause=n (default = 0) sets the seconds delay after starting subsequent nodes\n";
    print "--duration=n (default = 60) sets the seconds delay after starting the last node before shutting down the test\n";
    print "--time-stamp=s (default \"\") sets the timestamp in UTC for the genesis block. use \"now\" to mean the current time.\n";
    print "\nproducer count currently fixed at $prods\n";
    exit
}

die "pnodes value must be between 1 and $prods\n" if ($pnodes < 1 || $pnodes > $prods);

$nodes = $pnodes if ($nodes < $pnodes);

my $per_node = int ($prods / $pnodes);
my $extra = $prods - ($per_node * $pnodes);
my @pcount;
for (my $p = 0; $p < $pnodes; $p++) {
    $pcount[$p] = $per_node;
    if ($extra) {
        $pcount[$p]++;
        $extra--;
    }
}
my @pid;
my @data_dir;
my @p2p_port;
my @http_port;
my @peers;
$launch_pause = 0 if ($no_delay);
$first_pause = 0 if ($no_delay);

my $rhost = $hostname; # from a list for multihost tests
for (my $i = 0; $i < $nodes; $i++) {
    $p2p_port[$i] = $p2p_port_base + $i;
    $http_port[$i] = $http_port_base + $i;
    $data_dir[$i] = "$data_dir_base-$i";
}

opendir(DIR, ".") or die $!;
while (my $d = readdir(DIR)) {
    if ($d =~ $data_dir_base) {
        rmtree ($d) or die $!;
    }
}
closedir(DIR);

sub write_config {
    my $i = shift;
    my $producer = shift;

    mkdir ($data_dir[$i]);
    mkdir ($data_dir[$i]."/blocks");
    mkdir ($data_dir[$i]."/blockchain");

    open (my $cfg, '>', "$data_dir[$i]/config.ini") ;
    print $cfg "genesis-json = \"$genesis\"\n";
    print $cfg "block-log-dir = blocks\n";
    print $cfg "readonly = 0\n";
    print $cfg "shared-file-dir = blockchain\n";
    print $cfg "shared-file-size = 64\n";
    print $cfg "http-server-address = 127.0.0.1:$http_port[$i]\n";
    print $cfg "p2p-listen-endpoint = 0.0.0.0:$p2p_port[$i]\n";
    print $cfg "p2p-server-address = $hostname:$p2p_port[$i]\n";
    foreach my $peer (@peers) {
        print $cfg "p2p-peer-address = $peer\n";
    }

    if (defined $producer) {
        print $cfg "enable-stale-production = true\n";
        print $cfg "required-participation = true\n";
        print $cfg "private-key = [\"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV\",\"5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3\"]\n";

        print $cfg "plugin = eosio::producer_plugin\n";
        print $cfg "plugin = eosio::chain_api_plugin\n";

        my $prod_ndx = ord('a') + $producer;
        my $num_prod = $pcount[$producer];
        for (my $p = 0; $p < $num_prod; $p++) {
            print $cfg "producer-name = init" . chr($prod_ndx) . "\n";
            $prod_ndx += $pnodes; # ($p < $per_node-1) ? $pnodes : 1;
        }
    }
    close $cfg;
}

#connect each producer to the one ahead in the list, last to the first
#if bidir set then double connect to the one behind as well
sub make_ring_topology  {
    my $bidir = shift;
    if ($nodes == 1) {
        write_config (0,0);
        return 1;
    }

    for (my $i = 0; $i < $nodes; $i++) {
        my $pi = $i if ($i < $pnodes);
        my $rport = ($i == $nodes - 1) ? $p2p_port_base : $p2p_port[$i] + 1;
        $peers[0] = "$rhost:$rport";
        if ($nodes > 2 && $bidir) {
            $rport = $p2p_port[$i] - 1;
            $rport += $nodes if ($i == 0);
            $peers[1] = "$rhost:$rport";
        }
        write_config ($i, $pi);
    }
    return 1;
}

#connect each producer to three others, index+1, index + 1 + delta, index + 2(1 + delta)
#delta determined by total number of producer nodes. between 4 and 7, delta = pnodes-4
sub make_star_topology {
    if ($pnodes < 4) {
        return make_ring_topology(0);
    }
    my $numconn = 3;
    my @ports;
    my $delta = $pnodes > 6 ? 4 : $pnodes - $numconn;
    for (my $i = 0; $i < $nodes; $i++) {
        my $pi = $i if ($i < $pnodes);
        $ports[0] = $p2p_port[$i];
        for (my $d = 1; $d <= $numconn; $d++) {
            my $rind = ($i + ($d) * ($delta)) % $nodes;
            $rind++ if ($rind == $i);
            my $rport = $p2p_port[$rind];
            for (my $chk = 0; $chk < $d; $chk++) {
                if ($rport == $ports[$chk]) {
                    $rport++;
                    $chk = -1;
                }
            }
            $ports[$d] = $rport;
            $peers[$d-1] = "$rhost:$rport";
        }

        write_config ($i, $pi);
    }

    return 1;
 }

sub launch_nodes {
    my $gtsarg;
    if ($override_gts) {
        my $GTS = $override_gts;
        print "override gts = $override_gts\n";

        if ($override_gts =~ "now" ) {
            chomp ($GTS = `date -u "+%Y-%m-%dT%H:%M:%S"`);
            my @s = split (':',$GTS);
            $s[2] = substr ((100 + (int ($s[2]/3) * 3)),1);
            $GTS = join (':', @s);
            print "using genesis time stamp $GTS\n";
        }
        $gtsarg = "--genesis-timestamp=$GTS";
    }

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
            print "$cmdline[0], $cmdline[1], $cmdline[2]\n";
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

sub perform_work {
    my $mode = shift;
    sleep (5);
    print "all nodes launched, network running for $run_duration seconds\n";

    if ($mode == 0) {
        sleep ($run_duration);
    }
    elsif ($mode == 1) {
        my $stoptime = time () + $run_duration;
        my $counter = 0;
        while (time () < $stoptime) {
            `$eosc transfer eos inita 10 >> eosc.out 2>> eosc.err`;
            $counter++;
            if ($counter % 1000 == 0) {
                print "$counter client iterations\n";
            }
        }
        print "minimal contract ran $counter times.\n";
    }
    else {
        print "test mode $mode not defined\n";
    }
}

sub kill_nodes {
    foreach my $pp (@pid) {
        print "killing $pp\n";
        if (kill (2, $pp) != 0) {
            sleep (1);
            kill 9, $pp;
        }
    }
}


###################################################
# main

if    ( $topo =~ "ring" ) { make_ring_topology (1) or die; }
elsif ( $topo =~ "star" ) { make_star_topology () or die; }
else  { print "$topo is not a known topology" and die; }

launch_nodes ();

perform_work ($test);

kill_nodes () if ($run_duration > 0);
