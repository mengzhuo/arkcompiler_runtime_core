#!/usr/bin/env ruby
# Copyright (c) 2021-2022 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

require 'optparse'
require 'yaml'
require 'json'
require 'erb'

module Gen
  def self.on_require(data); end
end

require File.expand_path(File.join(File.dirname(__FILE__), 'runtime.rb'))

def create_sandbox
  # nothing but Ruby core libs and 'required' files
  binding
end

def check_option(optparser, options, key)
  return if options[key]

  puts "Missing option: --#{key}"
  puts optparser
  exit false
end

options = OpenStruct.new

optparser = OptionParser.new do |opts|
  opts.banner = 'Usage: gen.rb [options]'

  opts.on('-t', '--template FILE', 'Template for file generation (required)')
  opts.on('-d', '--datafiles FILE1,FILE2,FILE3', Array, 'Source data files in YAML format (required)')
  opts.on('-o', '--output FILE', 'Output file (required)')

  opts.on('-h', '--help', 'Prints this help') do
    puts opts
    exit
  end
end
optparser.parse!(into: options)

check_option(optparser, options, :datafiles)
check_option(optparser, options, :template)
check_option(optparser, options, :output)

template_file = File.read(File.expand_path(options.template))
output_file = File.open(File.expand_path(options.output), 'w')

output_file.puts('# Autogenerated file -- DO NOT EDIT!')

options.datafiles.each do |data|
  data = YAML.load_file(File.expand_path(data))
  data = JSON.parse(data.to_json, object_class: OpenStruct)
  Gen.on_require(data)

  t = ERB.new(template_file, nil, '%-')
  t.filename = options.template

  output_file.write(t.result(create_sandbox))
end

output_file.close
