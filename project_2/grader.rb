#!/usr/bin/env ruby

CURRENT_DIRECTORY   = `pwd`

if CURRENT_DIRECTORY   =~ /\/([^\/]+)\/project_/
  ECST_USERNAME       = $1
else
  puts "Cannot determine username"
  exit
end

BASE_DIR            = File.realpath(".")
SOURCE_PATH         = "csuc_http_#{ECST_USERNAME}.c"
HEADER_PATH         = "csuc_http.h"

TEST_DIR            = File.join(BASE_DIR, "tests")
HTML_DIR            = File.join(TEST_DIR, "doc_root")
INPUT_DIR           = File.join(TEST_DIR, "requests")
EXPECTED_OUTPUT_DIR = File.join(TEST_DIR, "expected_responses")
ACTUAL_OUTPUT_DIR   = File.join(TEST_DIR, "actual_responses")
MISC_DIR            = File.join(TEST_DIR, "misc")
ORIG_HEADER_PATH    = File.join(MISC_DIR, "csuc_http.h")
EXECUTABLE_DIR      = HTML_DIR
EXECUTABLE_NAME     = "csuc_http"
EXECUTABLE_PATH     = File.join(EXECUTABLE_DIR, EXECUTABLE_NAME)

TESTS = [{:input => "test_1.in", :expected_output => "expected_1.out", :args => "."},
         {:input => "test_2.in", :expected_output => "expected_2.out", :args => "."},
         {:input => "test_3.in", :expected_output => "expected_3.out", :args => "."},
         {:input => "test_4.in", :expected_output => "expected_4.out", :args => "."},
         {:input => "test_5.in", :expected_output => "expected_5.out", :args => "."},
         {:input => "test_6.in", :expected_output => "expected_6.out", :args => "."},
         {:input => "test_7.in", :expected_output => "expected_7.out", :args => "."},
         {:input => "test_8.in", :expected_output => "expected_8.out", :args => "more"},
         {:input => "test_9.in", :expected_output => "expected_9.out", :args => "."}]

puts "#{ECST_USERNAME}: #{SOURCE_PATH}"
puts
puts "Last modified datestamp:"
command = "git log -n1 #{SOURCE_PATH} | grep Date"
puts `#{command}`
puts

# Checksum the program
command = "cksum #{SOURCE_PATH}"
output  = `#{command}`
puts "CHECKSUM: #{output}"
puts

# Compile the program
puts "Compiling"
command = "gcc #{SOURCE_PATH} -o #{EXECUTABLE_PATH}"
puts `#{command}`
puts 

# Any differences between the header files
puts "Header File Changes"
puts `diff -b #{ORIG_HEADER_PATH} #{HEADER_PATH}`
puts
puts

`mkdir -p #{ACTUAL_OUTPUT_DIR}`

$stdin.gets

TESTS.each_with_index do |test, index|
  input_path           = File.join(INPUT_DIR, test[:input])
  expected_output_path = File.join(EXPECTED_OUTPUT_DIR, test[:expected_output])
  actual_output_path   = File.join(ACTUAL_OUTPUT_DIR, "test_#{index + 1}.out")
  actual_error_path    = File.join(ACTUAL_OUTPUT_DIR, "test_#{index + 1}.err")
  command              = "cd #{EXECUTABLE_DIR}; ./#{EXECUTABLE_NAME} #{test[:args]} < #{input_path} > #{actual_output_path} 2> #{actual_error_path}"
  
  puts "TEST #{index + 1}"
  `#{command}`
  
  puts
  puts "Output diff:"
  puts `diff -a #{expected_output_path} #{actual_output_path}`
  puts    
  puts "Output diff (ignoring line endings):"
  puts `diff -a --strip-trailing-cr #{expected_output_path} #{actual_output_path}`
  puts
  puts "Error output:"
  puts `head #{actual_error_path}`
  puts
  puts "Standard output:"
  puts `head #{actual_output_path}`
  puts
  $stdin.gets
end
