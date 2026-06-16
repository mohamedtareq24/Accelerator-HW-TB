# Load radix-16 COE stimulus into axi_bram_src (PS/DMA view).
#
# Usage (xsct):
#   connect
#   targets -set <a53_core>
#   source <repo>/stimulus/load_stimulus.tcl
#
# Optional overrides before sourcing:
#   set ::stimulus_coe "/path/to/custom.coe"
#   set ::stimulus_base 0xA0002000

set script_dir [file dirname [file normalize [info script]]]

if {![info exists ::stimulus_coe]} {
	set ::stimulus_coe [file join $script_dir "src_stimulus.coe"]
}
if {![info exists ::stimulus_base]} {
	set ::stimulus_base 0xA0002000
}

if {![file exists $::stimulus_coe]} {
	error "COE file not found: $::stimulus_coe"
}

set fp [open $::stimulus_coe r]
set addr $::stimulus_base
set count 0

while {[gets $fp line] >= 0} {
	if {[string match "*=*" $line]} {
		continue
	}

	set val [string trim $line ",; \r\n\t"]
	if {$val eq ""} {
		continue
	}

	mwr $addr 0x$val
	incr addr 4
	incr count
}

close $fp

puts "Loaded $count words from $::stimulus_coe"
puts "  base  [format 0x%08X $::stimulus_base]"
puts "  end   [format 0x%08X [expr {$addr - 4}]]"
puts "Verify: mrd $::stimulus_base 4"
