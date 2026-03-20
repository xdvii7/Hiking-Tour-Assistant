from flask import Flask
from flask import render_template
from flask import jsonify
from flask import Response
from flask import redirect

import db
import hike

app = Flask(__name__)
hdb = db.HubDatabase()

@app.route('/')
def get_home():
    sessions = hdb.get_sessions()

    total_km = 0.0
    total_steps = 0
    total_kcal = 0.0

    for s in sessions:
        total_km += s.km
        total_steps += s.steps
        total_kcal += s.kcal
        
    return render_template('home.html', sessions=sessions,total_km = total_km, total_steps = total_steps, total_kcal = total_kcal)

@app.route('/sessions')
def get_sessions():
    sessions = hdb.get_sessions() 
    sessions = list(map(lambda s: hike.to_list(s), sessions))
    print(sessions)
    return jsonify(sessions)

@app.route('/sessions/<id>')
def get_session_by_id(id):
    session = hdb.get_session(id)
    return jsonify(hike.to_list(session))


@app.route('/sessions/<id>/delete', methods=["POST"])
def delete_session(id):
    id = int(id)
    hdb.delete(id)
    print(f'DELETED SESSION WITH ID: {id}')
    return redirect("/")
    
if __name__ == "__main__":
    app.run('0.0.0.0', debug=True)
