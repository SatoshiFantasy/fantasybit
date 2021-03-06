//
//  sfgui.cpp
//  cute-fantasy
//
//  Created by Jay Berg on 4/8/14.
//
//
#include "sfgui.h"
#include "client.h"
#include <ProtoData.pb.h>
#include "ui_sfgui.h"
#include "teamsloader.h"
#include "playerloader.h"

#include <QDateTime>
#include <QClipboard>
#include <QItemEditorFactory>

using namespace fantasybit;

sfGUI::sfGUI(QWidget *parent) : QWidget(parent), ui(new Ui::sfGUI)
{
    ui->setupUi(this);
    //QObject::connect(this, SIGNAL(on_flash()), this, SLOT(flashing()) );
    ui->textBrowser->append(QCoreApplication::applicationDirPath());

   ui->myTeamsStatesTableView->setModel(&myTeamsStateTableModel);
   ui->myPlayersDataTableView->setModel(&myPlayerDataTableModel);
   ui->myTeamsDataTableView->setModel(&myTeamDataTableModel);
   ui->myFantasyPlayersTableView->setModel(&myFantasyPlayerTableModel);
   ui->myScoringTableView->setItemDelegateForColumn(2,&myDelegate);
   myScoringTableModel.setEditable(true);
   ui->myScoringTableView->setModel(&myScoringTableModel);
   myScoringTableModel.setAutoDelete(true);

   TeamLoader teamLoader;
   PlayerLoader playerLoader;
   QString error ;
   bool success = teamLoader.loadTeamsFromJsonFile(myPreloadedTeams,error);
   if (!success)
       QMessageBox::critical(this,"Loading teams from JSon File",error);

   success = playerLoader.loadPlayersFromJsonFile(myPreloadedPlayers,error);
   if (!success)
       QMessageBox::critical(this,"Loading players from JSon File",error);


   /*
    if (!success)
        QMessageBox::critical(this,"Loading teams from JSon File",error);
    else {
        for(int i=0;i<myPreloadedTeams.size();i++){
            TeamLoader::JsonTeam team = myPreloadedTeams[i];
            ui->myTeamsCmb->addItem(team.myKey +":"+  team.myFullName, QVariant(team.myKey));
        }
    }
    */

    for (int i = DataTransition::Type_MIN; i < DataTransition::Type_ARRAYSIZE; i++) {

        if (!DataTransition::Type_IsValid(i)) continue;

        ui->dataTransitionCombo->addItem(
                    QString::fromStdString(DataTransition::Type_Name(DataTransition::Type(i))),
                                         QVariant(i));
    }


}

/*
void sfGUI::flashing()
{
    ui->progressBar->setVisible(true);
}
*/

void sfGUI::fromServer(const DeltaData &in)
{
    if ( in.type() == DeltaData_Type_HEARTBEAT )
    {
        ui->textBrowser->insertPlainText(".");
        deltaData.CopyFrom(in);
        updatedelta();
    }
    else if ( in.type() == DeltaData_Type_SNAPSHOT)
    {
        if ( !snapData.has_type() ) {
            ui->textBrowser->append(QDateTime::currentDateTime().toString() + " Connected");

            ui->message->setText("Live: ");
            ui->fantasyname->setEnabled(true);
            ui->generate->setEnabled(true);
        }

        snapData.CopyFrom(in);
        updatesnap();
    }

    ui->textBrowser->append(QString::fromStdString(in.DebugString()));

    refreshViews(in);
}
/**/

void sfGUI::updatesnap()
{
    if ( snapData.myfantasyname().status() != MyNameStatus::none ) {
        ui->fantasyname->setText(QString::fromStdString(snapData.myfantasyname().name()));
        ui->fantasynamestatus->setText("Fantasy Name Status: " +
            QString::fromStdString(MyNameStatus_Name(snapData.myfantasyname().status())));
    }


    ui->message->setText(QString::fromStdString(
               "Live: " +
                GlobalState_State_Name(snapData.globalstate().state()) +
                " " +
                std::to_string(snapData.globalstate().season())
                ));
}

void sfGUI::updatedelta()
{

}

sfGUI::~sfGUI()
{
    delete ui;
}


void fantasybit::sfGUI::on_generate_clicked()
{

    if ( ui->fantasyname->text() == "") return;

    ui->textBrowser->append(QDateTime::currentDateTime().toString() + " requesting: " + ui->fantasyname->text());

    indata.set_type(InData_Type_NEWNAME);
    indata.set_data(ui->fantasyname->text().toStdString());

    emit fromGUI(indata);
}


void fantasybit::sfGUI::on_copy_clicked()
{
    /*
    QClipboard *clipboard = QApplication::clipboard();
    if ( ui->twitter->isChecked() )
    {
        QString tweet("@satoshifantsy (");
        tweet += m_namestatus.name().c_str();
        tweet += ") (";
        tweet += QString::number(m_namestatus.nametransaction().hash()),
        tweet += ") (";
        tweet += m_namestatus.nametransaction().sigid().c_str();
        tweet += ")";
        ui->proofofwork->append(tweet);
        clipboard->setText(tweet);
        ui->message->setText("Proof copied to clipboard, please paste ad tweet");
    }
    else
    {
        clipboard->setText(ui->proofofwork->toPlainText());
        ui->message->setText("Proof copied to clipboard");
    }
    */
}

void fantasybit::sfGUI::refreshViews(const DeltaData &in){
    //QMutexLocker locker(&myMutex);
    if (in.type() == DeltaData_Type_SNAPSHOT) {
        myTeamsStateTableModel.removeAll();
        myPlayerDataTableModel.removeAll();
        myTeamDataTableModel.removeAll();
        myFantasyPlayerTableModel.removeAll();
    }

    myCurrentSnapShot.fromDeltaData(in);
    ui->mySnapshotTimestamp->setText(QDateTime::currentDateTime().toString(Qt::ISODate));
    ui->myFantasyNameLE->setText(myCurrentSnapShot.fantasyName);
    ui->myBalance->setText(QString::number(myCurrentSnapShot.fantasyNameBalance));
    ui->mySeasonLE->setText(myCurrentSnapShot.globalStateModel.seasonString());
    ui->myGlobalStateLE->setText(myCurrentSnapShot.globalStateModel.stateString());


    for(int i=0;i< myCurrentSnapShot.teamStates.count();i++)
        myTeamsStateTableModel.addItem(&myCurrentSnapShot.teamStates[i]);    

    for(int i=0;i< myCurrentSnapShot.players.count();i++)
        myPlayerDataTableModel.addItem(&myCurrentSnapShot.players[i]);

    for(int i=0;i< myCurrentSnapShot.teams.count();i++) {
        myTeamDataTableModel.addItem(&myCurrentSnapShot.teams[i]);
        ui->myTeamsCmb->addItem(myCurrentSnapShot.teams[i].teamId(), QVariant(myCurrentSnapShot.teams[i].teamId()));
    }

    for(int i=0;i< myCurrentSnapShot.fantasyPlayers.count();i++)
        myFantasyPlayerTableModel.addItem(&myCurrentSnapShot.fantasyPlayers[i]);
}

void fantasybit::sfGUI::on_myTeamsCmb_currentIndexChanged(int index)
{
   QString teamKey =  ui->myTeamsCmb->itemData(index).toString();
   //reload players combo
   ui->myPlayersCmb->clear();
   foreach(PlayerDataViewModel * player, myPlayerDataTableModel.list()){
       if (player->teamId().trimmed().toUpper()== teamKey.trimmed().toUpper())
        ui->myPlayersCmb->addItem(player->playerId(),QVariant(player->playerId()));
   }
}

void fantasybit::sfGUI::on_myAddScoringLineButton_clicked()
{
    if (ui->myTeamsCmb->currentData().isValid() &&
            ui->myPlayersCmb->currentData().isValid()){
       QString teamId,teamName,playerId;
       teamId = ui->myTeamsCmb->currentData().toString();
       teamName = ui->myTeamsCmb->currentText();
       playerId = ui->myPlayersCmb->currentData().toString();

       //check if the player is already scored
       bool added = false;
       foreach(ScoringModelView * scoring,myScoringTableModel.list() ) {
           if (scoring->myTeamId.trimmed().toUpper() == teamId.trimmed().toUpper()&&
               scoring->myPlayerId.trimmed().toUpper() == playerId.trimmed().toUpper()){
               added = true;
               break;
           }
       }
       if (!added)
           myScoringTableModel.addItem(new ScoringModelView(teamId,teamName,playerId));
       else
           QMessageBox::critical(this,"Cute Fantasy","You've already scored the selected player!");

    }
    else
        QMessageBox::critical(this,"Cute Fantasy","Please select a team and a player before adding a new line!");

}

void fantasybit::sfGUI::on_myAddTeam_clicked()
{
    if (ui->myTeamsCmb->currentData().isValid() )
    {
        ui->teamStartList->addItem(ui->myTeamsCmb->currentData().toString());
        myTeamTransitions.append(ui->myTeamsCmb->currentData().toString());
    }
}


void fantasybit::sfGUI::on_mySendProjectionsButton_clicked()
{
    foreach(ScoringModelView * scoring,myScoringTableModel.list() ) {
        indata.Clear();
        indata.set_type(InData_Type_PROJ);
        indata.set_data2(scoring->myPlayerId.toStdString());
        indata.set_num(scoring->myScore);
        emit fromGUI(indata);
    }
}

void fantasybit::sfGUI::on_mySendResultsButton_clicked()
{
    DataTransition dt{};

    dt.set_type(static_cast<DataTransition_Type>(ui->dataTransitionCombo->currentData().toInt()));
    dt.set_season(ui->seasonBox->value());
    dt.set_week(ui->weekBox->value());

    for (auto st :  myTeamTransitions) {
        dt.add_teamid(st.toStdString());
    }

    foreach(ScoringModelView * scoring,myScoringTableModel.list() ) {
        Data d{};
        d.set_type(Data::RESULT);
        ::fantasybit::FantasyPlayerPoints fpp{};
        fpp.set_season(dt.season());
        fpp.set_week(dt.week());
        fpp.set_playerid(scoring->myPlayerId.toStdString());
        fpp.set_points(scoring->myScore);
        ResultData rd{};
        rd.mutable_fpp()->CopyFrom(fpp);

        d.MutableExtension(ResultData::result_data)->CopyFrom(rd);
        Data *d2 = dt.add_data();
        d2->CopyFrom(d);
    }

    if ( ui->allTeams->isChecked() ) {
        Data d{};
        d.set_type(Data::TEAM);
        TeamData td{};
        for ( auto t : myPreloadedTeams ) {
            td.set_teamid(t.myKey.toStdString());
            d.MutableExtension(TeamData::team_data)->CopyFrom(td);
            Data *d2 = dt.add_data();
            d2->CopyFrom(d);
        }
    }

    if ( ui->allPLayers->isChecked() ) {
        Data d{};
        d.set_type(Data::PLAYER);
        PlayerData td{};
        for ( auto t : myPreloadedPlayers ) {
            td.set_teamid(t.Team.toStdString());
            td.set_playerid(t.PlayerID.toStdString());
            d.MutableExtension(PlayerData::player_data)->CopyFrom(td);
            Data *d2 = dt.add_data();
            d2->CopyFrom(d);
        }
    }

    indata.set_type(InData_Type_DATA);
    indata.mutable_data_trans()->CopyFrom(dt);

    emit fromGUI(indata);
}

void fantasybit::sfGUI::on_myDeleteAllRowsButton_clicked()
{
    myScoringTableModel.removeAll();
    myTeamTransitions.clear();
    ui->teamStartList->clear();
}

