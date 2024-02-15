#!/bin/env ruby

# Copyright (c) 2024 Huawei Device Co., Ltd.
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

pure_binding = TOPLEVEL_BINDING.dup

require 'yaml'
require 'erb'
require 'json'
require 'ostruct'
require 'open3'
require 'fileutils'
require 'optparse'
require 'set'

require_relative 'src/types'
require_relative 'src/value_dumper'

$options = {
    :'chunk-size' => 200,
    :proc => 8,
    :'ts-node' => ['npx', 'ts-node'],
    :'filter' => /.*/
}

module EXIT_CODES
    OK = true
    INVALID_OPTIONS = false
    INVALID_NODE_VERSION = false
    TESTS_FAILED = false
end

def print_help
    puts $optparse
    puts 'NOTE: this program requires node version 21.4, you can use `n` tool to install it'
end

$optparse = OptionParser.new do |opts|
    opts.banner = 'Usage: test-generator [options] --out=DIR --tmp=DIR confs...'
    opts.on '--run-ets=PANDA', 'used to instantly run es2panda&ark on generated file, PANDA is a path to panda build directory'
    opts.on '--out=DIR', String, 'path to .ets files output directory'
    opts.on '--tmp=DIR', String, 'path to temporary directory (where to output .ts and .json files)'
    opts.on '--chunk-size=NUM', Integer, 'amout of tests in a single file'
    opts.on '--proc=PROC', Integer, 'number of ruby threads to use, defaults to 8' do |v|
        $options[:proc] = [v, 1].max
    end
    opts.on '--ts-node=PATH', String, 'colon (:) separated list of prefix arguments to run ts-node, defaults to npx:ts-node' do |v|
        $options[:'ts-node'] = v.split(':')
    end
    opts.on '--filter=REGEXP', Regexp, 'test name filter; name consists of categories and method/expr (escaped) strings'
    opts.on '--help' do
        print_help
        exit EXIT_CODES::OK
    end
end
$optparse.parse!(into: $options)

def check_opt(name)
    if $options[name].nil?
        puts "--#{name} is not provided"
        puts $optparse
        exit EXIT_CODES::INVALID_OPTIONS
    end
end

check_opt :out
check_opt :tmp

FileUtils.rm_rf $options[:out]
FileUtils.mkdir_p $options[:out]
FileUtils.mkdir_p $options[:tmp]

$user_binding = pure_binding.dup
eval("require '#{__dir__}/src/script_module'", $user_binding)
ScriptClass = Class.new.extend(eval('ScriptModule', $user_binding))

$template_ts = ERB.new File.read("#{__dir__}/templates/template.ts.erb"), nil, '%'
$template_ets = ERB.new File.read("#{__dir__}/templates/template.ets.erb"), nil, '%'

def deep_copy(a)
    Marshal.load(Marshal.dump(a))
end

class ThreadPool
    def initialize(threads_num)
        @threads_num = threads_num
        @threads = Set.new
        @threads_done = Set.new
    end

    def run(*args, &block)
        # wait while at least 1 task completes
        while @threads.size >= @threads_num
            sleep 0.05
        end
        t = Thread.new do
            me = Thread.current
            @threads << me
            yield *args
        rescue => e
          puts e.full_message(highlight: true, order: :top)
        ensure
            @threads_done << me
            @threads.delete me
        end
        @threads << t
    end

    def join()
        while @threads.size != 0
            sleep 0.15
            @threads.subtract(@threads_done)
        end
    end
end

class Generator
    attr_reader :test_count, :tests_failed, :test_files, :tests_ignored

    def initialize(filter_pattern)
        @filter_pattern = filter_pattern

        @conf = OpenStruct.new
        @conf.self = nil
        @conf.vars = eval('Vars.new', $user_binding)
        @conf.category = ""

        @tests_by_name = {}

        @test_count = 0
        @test_files = 0
        @tests_failed = 0
        @tests_ignored = 0

        check_node_version
    end

    def check_node_version()
        begin
            version = get_command_output(*$options[:'ts-node'], '-e', 'console.log(process.versions.node)')
        rescue
            puts 'Failed to check node version. Make sure that you both installed node and ran `npm install` within generator dir'
            puts "Autodetected generator dir: '#{__dir__}'"
            puts
            print_help
            raise
        end
        unless version =~ /21\.4(\..*|\s*$)/
            puts "Invalid node version #{version}"
            puts
            print_help
            exit EXIT_CODES::INVALID_NODE_VERSION
        end
    end

    def process(sub)
        old_conf = deep_copy(@conf)
        conf = @conf
        if sub["ignored"]
            @tests_ignored += 1
            return
        end
        if sub["category"]
            conf.category += sub["category"] + "_"
        end
        if sub.has_key?("self")
            conf.self = sub["self"]
            conf.self_type = sub["self_type"]
        end
        (sub["vars"] || {}).each { |k, v|
            conf.vars[k] = v
        }
        (sub["sub"] || []).each { |s|
            process(s)
        }
        if sub["method"] || sub["expr"]
            conf.ret_type = sub["ret_type"]
            if sub["expr"]
                name = conf.category + sub["expr"].gsub(/[^a-zA-Z0-9_]/, '_')
                is_expr = true
            else
                name = conf.category + sub["method"]
                is_expr = false
            end
            return if not (@filter_pattern =~ name)
            mandatory = sub["mandatory"]
            mandatory ||= -1
            rest = (sub["rest"] || ["emptyRest"]).map { |p| eval(p, conf.vars.instance_eval { binding }) }
            pars = (sub["params"] || []).map { |p| eval(p, conf.vars.instance_eval { binding }) }
            tests = @tests_by_name[name] || []
            @tests_by_name[name] = tests
            add = []
            if conf.self
                add = [ScriptClass.paramOf(*conf.self.map { |s| s.strip })]
                if mandatory != -1
                    mandatory += 1
                end
            end
            gen_params(add + pars, mandatory: mandatory, rest_supp: rest).each { |pars|
                push = OpenStruct.new
                push.conf = conf
                push.self = conf.self
                push.ts = OpenStruct.new
                push.ets = OpenStruct.new
                if conf.self
                    raise if pars.ts.size < 1
                    raise if pars.ets.size < 1
                    push.ts.self = pars.ts[0]
                    push.ets.self = pars.ets[0]
                    pars.ts = pars.ts[1..]
                    pars.ets = pars.ets[1..]
                end
                if is_expr
                    push.ts.expr = sub["expr"].gsub(/\bpars\b/, pars.ts.join(', '))
                    push.ets.expr = sub["expr"].gsub(/\bpars\b/, pars.ets.join(', '))
                else
                    slf = conf.self ? "self." : ""
                    push.ts.expr = "#{slf}#{sub["method"]}(#{pars.ts.join(', ')})"
                    push.ets.expr = "#{slf}#{sub["method"]}(#{pars.ets.join(', ')})"
                end
                tests.push(push)
            }
        end
    ensure
        @conf = old_conf
    end

    def run()
        running = Set.new
        deleted = Set.new
        thread_pool = ThreadPool.new($options[:proc])
        @tests_by_name.each { |k, v|
            thread_pool.run(k, v) do |k, v|
                run_test k, v
            rescue
                @tests_failed += 1
                raise
            end
        }
        thread_pool.join
    end

    def get_command_output(*args)
        stdout_str, status = Open3.capture2e(*args)
        if status != 0
            puts stdout_str
            raise "invalid status #{status}\ncommand: #{args.join(' ')}\n\n#{stdout_str}"
        end
        stdout_str
    end

    def run_test(k, test_cases)
        @test_count += test_cases.size
        test_cases.each_slice($options[:'chunk-size']).with_index { |test_cases_current_chunk, chunk_id|
            puts "#{k} #{chunk_id}"
            @test_files += 1

            # ts part
            ts_path = File.join $options[:tmp], "#{k}#{chunk_id}.ts"
            buf = $template_ts.result(binding)
            File.write ts_path, buf
            puts "run  ts for #{k} #{chunk_id}"
            stdout_str = get_command_output(*$options[:'ts-node'], ts_path)
            File.write(File.join($options[:tmp], "#{k}#{chunk_id}.json"), stdout_str)

            # ets part
            expected = JSON.load(stdout_str)
            buf = $template_ets.result(binding)
            ets_path = File.join $options[:out], "#{k}#{chunk_id}.ets"
            File.write ets_path, buf

            # verify ets
            if $options[:"run-ets"]
                puts "run ets for #{k} #{chunk_id}"
                panda_build = $options[:"run-ets"]
                abc_path = File.join $options[:tmp], "#{k}#{chunk_id}.abc"
                get_command_output("#{panda_build}/bin/es2panda", "--extension=ets", "--opt-level=2", "--output", abc_path, ets_path)
                get_command_output("#{panda_build}/bin/ark", "--no-async-jit=true", "--gc-trigger-type=debug", "--boot-panda-files", "#{panda_build}/plugins/ets/etsstdlib.abc", "--load-runtimes=ets", abc_path, "ETSGLOBAL::main")
                puts "✔✔✔ ets for #{k} #{chunk_id}"
            end
        }
    rescue
        raise $!.class, "ruby: failed #{k}: #{$!}"
    end

    def gen_params(types_supp, rest_supp: [], mandatory: -1)
        if mandatory < 0
            mandatory = types_supp.size
        end
        res = [[]]
        types_supp.each_with_index { |type_supp, idx|
            res = res.flat_map { |old|
                type_supp.().flat_map { |y|
                    if old.size == idx
                        [old + [y]] + (idx < mandatory ? [] : [old])
                    else
                        [old]
                    end
                }
            }
        }
        res.uniq!
        new_res = []
        if rest_supp.size > 0
            rest_supp.each { |rest_one_supp|
                added_cases = rest_one_supp.().flat_map { |r|
                    res.map { |o| o.size == types_supp.size ? o + r : o.dup }
                }
                new_res.concat(added_cases)
            }
        end
        res = new_res
        res.map! { |r|
            p = OpenStruct.new
            p.ts = deep_copy(r)
            p.ets = r
            # NOTE(kprokopenko): legacy code for rest parameters,
            # remove after dev branch and everything else is merged into master
            while r.size < types_supp.size
                r.push "undefined"
            end
            p
        }
        return res
    end
end

gen = Generator.new($options[:'filter'])

ARGV.each { |a|
    puts "reading #{a}"
    file = YAML.load_file(a)
    gen.process file
}

gen.run

puts "total tests: #{gen.test_count}"
if $options[:'run-ets']
    puts "failed files: #{gen.tests_failed}/#{gen.test_files}"
end
puts "ignored subtrees: #{gen.tests_ignored}"

if gen.tests_failed != 0
    puts "Some of tests failed"
    exit EXIT_CODES::TESTS_FAILED
end