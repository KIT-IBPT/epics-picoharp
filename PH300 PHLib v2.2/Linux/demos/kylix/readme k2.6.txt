  Problem Description: 1. Full Product version(s): Kylix 3 all 2. Full Platform version: 
  Linux 2.6.x kernel versions ( Suse9.1/Mandrake10.0/RHES3.x/Fedora Core2/...) 
  If installing Kylix 3 on a Linux 2.6.x platform can be done generally without problem,
  when starting the application or debugging you may have some issues: IDE hangs. 
  To avoid this issue, you need to verify first some settings that will ensure that your
  system will knows where to look for the libraries... this can be done by editing the
  /etc/ld.so.con file and add your kylix install directory path. Typically this is 
  /usr/local/kylix3/bin
  Then you need to run "ldconfig" in Super user mode. Once this is done, you will need 
  to add the following in the startdelphi and/or startbcb script: 
  "export LD_ASSUME_KERNEL=2.4.1 would make the debugger work without a problem."
  Solution created by pdessart@borland.com
  