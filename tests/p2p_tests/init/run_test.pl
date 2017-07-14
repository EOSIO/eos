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

my $first_pause = 45;
my $launch_pause = 5;
my $run_duration = 60;

if (!GetOptions("nodes=i" => \$nodes,
                "first-pause=i" => \$first_pause,
                "launch-pause=i" => \$launch_pause,
                "duration=i" => \$run_duratiion,
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
my $prod_ndx = ord('a');
for (my $i = 0; $i < $nodes; $i++) {
    my $p2p_port = $p2p_port_base + $i;
    my $forward = $p2p_port + 1;
    my $backward = $p2p_port - 1;
    my $http_port = $http_port_base + $i;
    my $data_dir = "$data_dir_base-$i";
    if ($nodes > 1) {
        if ($i == 0) {
            $backward = $p2p_port_base + $nodes -1;
        }
        elsif ($i == $nodes - 1) {
            $forward = $p2p_port_base;
        }
    }
    print "purging directory $data_dir\n";
    rmtree ($data_dir);
    mkdir ($data_dir);

    open (my $cfg, '>', "$data_dir/config.ini") ;
    print $cfg "genesis-json = \"$genesis\"\n";
    print $cfg "block-log-dir = \"blocks\"\n";
    print $cfg "readonly = 0\n";
    print $cfg "shared-file-dir = \"blockchain\"\n";
    print $cfg "shared-file-size = 64\n";
    print $cfg "http-server-endpoint = 127.0.0.1:$http_port\n";
    print $cfg "listen-endpoint = 127.0.0.1:$p2p_port\n";
    print $cfg "remote-endpoint = 127.0.0.1:$forward\n" if ($nodes > 1);
    print $cfg "remote-endpoint = 127.0.0.1:$backward\n" if ($nodes > 2);
    print $cfg "public-endpoint = 0.0.0.0:$p2p_port\n";
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

    my @cmdline = ($eosd,
#                   "--genesis-timestamp=now",
                   "--data-dir=$data_dir");

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
        open OUTPUT, '>', "$data_dir/stdout.txt" or die $!;
        open ERROR, '>', "$data_dir/stderr.txt" or die $!;
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

print "all nodes launched, network running for $run_duration seconds\n";
sleep ($run_duration);
foreach my $pp (@pid) {
    print "killing $pp\n";
    my $res = kill 2, $pp;
    print "kill returned $res"
}
