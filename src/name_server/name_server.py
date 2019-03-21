from flask import *
from flask_restful import Resource, Api

app = Flask(__name__)
api = Api(app)

addr_to_info = {}
name_to_info = {}

#/server/username will return a json of all the users info
class GetInfoByAddress(Resource):
    def get(self, address):
        print("getting info for addr: "+address)
        try:
            name, port = addr_to_info[address]
            print("name: "+name+" port: "+port) 
            return name+":"+port 
        except:
            print("no info found for that addr")
            return "0"

class GetInfoByName(Resource):
    def get(self, name):
        print("getting info for name: "+name)
        try:
            addr, port = name_to_info[name]
            print("addr: "+addr+" port: "+port) 
            return addr+":"+port
        except:
            print("no info found for that name")
            return "0"

class PostInfoForRouter(Resource):
    def post(self, name, addr, port):
        print("adding info for "+name+" "+addr+" "+port)
        name_to_info[name] = (addr, port)
        addr_to_info[addr] = (name, port)

class RemoveRouter(Resource):
    def delete(self, addr):
        print("deleting info for router "+addr)
        try:
            name, port = addr_to_info[addr]
            del addr_to_info[addr]
            del name_to_info[name]
            return "1"
        except:
            return "0"

class FlushAll(Resource):
    def get(self):
        print("deleting all info")
        addr_to_info.clear()
        name_to_info.clear()


api.add_resource(GetInfoByAddress, '/addr/<string:address>')
api.add_resource(GetInfoByName, '/name/<string:name>')
api.add_resource(PostInfoForRouter, '/add/<string:name>/<string:addr>/<string:port>')
api.add_resource(RemoveRouter, '/remove/<string:addr>')
api.add_resource(FlushAll,'/flush/')

if __name__ == '__main__':
    app.run(debug=True)
