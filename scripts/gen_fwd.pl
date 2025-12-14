#!/usr/bin/env perl
use strict;
use warnings;
use File::Find;

my $root = shift // "src";
my %namespaces;
my @ns_stack;

sub record_decl {
    my ($ns, $kind, $name) = @_;
    push @{ $namespaces{$ns} }, "$kind $name;";
}

find(
    {
        wanted => sub {
            return unless /\.(h|hpp)$/;
            open my $fh, "<", $_ or return;

            @ns_stack = ();
            while (my $line = <$fh>) {
                # strip comments
                $line =~ s{//.*$}{};
                $line =~ s{/\*.*?\*/}{}g;

                # namespace open
                if ($line =~ /^\s*namespace\s+([\w:]+)\s*\{/) {
                    push @ns_stack, $1;
                    next;
                }

                # namespace close
                if ($line =~ /^\s*\}/) {
                    pop @ns_stack if @ns_stack;
                    next;
                }

                # class / struct (ignore templates)
                if ($line =~ /^\s*(class|struct)\s+(\w+)/) {
                    my ($kind, $name) = ($1, $2);
                    my $ns = join("::", @ns_stack) || "";
                    record_decl($ns, $kind, $name);
                }
            }
            close $fh;
        },
        no_chdir => 1,
    },
    $root
);

# Emit file
print "#pragma once\n\n";
print "// AUTO-GENERATED. EDIT FREELY AFTER FIRST RUN.\n\n";

for my $ns (sort keys %namespaces) {
    if ($ns ne "") {
        print "namespace $ns {\n";
    }

    my %seen;
    for my $decl (@{ $namespaces{$ns} }) {
        next if $seen{$decl}++;
        print "  $decl\n";
    }

    if ($ns ne "") {
        print "}\n";
    }
    print "\n";
}

