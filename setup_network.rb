require "open3"

config_fn = ARGV.first
if config_fn.nil?
  puts "specify config file, and optionally a second to spawn dockers"
  return 1
end

if ARGV[1]
  puts "spinning up docker images"
  setup_docker
end

puts "reading config from #{config_fn}"

section = "";

File.readlines(config_fn).each do |line|
  if line.match(/\[*\]/)
    section = line.gsub("]", "").gsub("[", "")
  elsif not section.empty?
    puts section
    stdin, stdout, stderr = Open3.popen3("./src/ui #{section}")
    stdin.puts "1"
    stdin.puts line
    stdin.close_write
    stdout.each_line { |line| puts line }
  end
end
